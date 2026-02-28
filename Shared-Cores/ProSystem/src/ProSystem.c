// ----------------------------------------------------------------------------
//   ___  ___  ___  ___       ___  ____  ___  _  _      __  ___
//  /__/ /__/ /  / /__  /__/ /__    /   /_   / |/ /      / / _
// /    / \  /__/ ___/ ___/ ___/   /   /__  /    /   ___/ /__/
//
// ----------------------------------------------------------------------------
// Copyright 2003, 2004 Greg Stanton
// Copyright 2020 Rupert Carmichael
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// ----------------------------------------------------------------------------
// ProSystem.c
// ----------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ProSystem.h"
#define PRO_SYSTEM_STATE_HEADER "PRO-SYSTEM STATE"
#define PRO_SYSTEM_SOURCE "ProSystem.c"

bool prosystem_active = false;
bool prosystem_paused = false;
uint16_t prosystem_frequency = 60;
uint8_t prosystem_frame = 0;
uint16_t prosystem_scanlines = 262;
uint32_t prosystem_cycles = 0;
uint32_t prosystem_extra_cycles = 0;
int lightgun_scanline = 0;
float lightgun_cycle = 0;

// Whether the last CPU operation resulted in a half cycle (need to take it
// into consideration)
extern bool half_cycle;

void prosystem_Reset(void) {
    if (cartridge_IsLoaded()) {
        prosystem_paused = false;
        prosystem_frame = 0;
        sally_Reset();
        region_Reset();
        tia_Clear();
        tia_Reset();
        pokey_Clear();
        pokey_Reset();
        xm_Reset();
        memory_Reset();
        maria_Clear();
        maria_Reset();
        riot_Reset();
        
        if (bios_enabled) {
            bios_Store();
        }
        else {
            cartridge_Store();
        }
        
        prosystem_cycles = sally_ExecuteRES();
        prosystem_active = true;
    }
}

// Strobe based on the current lightgun location
static void prosystem_FireLightGun(void) {
    if (((maria_scanline >= lightgun_scanline) &&
        (maria_scanline <= ( lightgun_scanline + 3 ))) &&
        ((int)prosystem_cycles >= ((int)lightgun_cycle ) - 1 )) {
        memory_ram[INPT4] &= 0x7f;
    }
    else {
        memory_ram[INPT4] |= 0x80;
    }
}

void prosystem_ExecuteFrame(const uint8_t* input) {
    // Is WSYNC enabled for the current frame?
    bool wsync = !(cartridge_flags & CARTRIDGE_WSYNC_MASK);
    
    // Is Maria cycle stealing enabled for the current frame?
    bool cycle_stealing = !(cartridge_flags & CARTRIDGE_CYCLE_STEALING_MASK);
    
    // Is the lightgun enabled for the current frame?
    bool lightgun = ((cartridge_controller[0] & CARTRIDGE_CONTROLLER_LIGHTGUN) && (memory_ram[CTRL] & 96) != 64);
    
    riot_SetInput(input);
    
    prosystem_extra_cycles = 0;
    
    if (cartridge_pokey || cartridge_xm) pokey_Frame();
    
    for (maria_scanline = 1; maria_scanline <= prosystem_scanlines; maria_scanline++) {
        if (maria_scanline == maria_displayArea.top) {
            memory_ram[MSTAT] = 0;
        }
        else if(maria_scanline == maria_displayArea.bottom) {
            memory_ram[MSTAT] = 128;
        }
        
        // Was a WSYNC performed withing the current scanline?
        bool wsync_scanline = false;
        
        uint32_t cycles = 0;
        
        if (!cycle_stealing || (memory_ram[CTRL] & 96 ) != 64) {
            // Exact cycle counts when Maria is disabled
            prosystem_cycles %= CYCLES_PER_SCANLINE;
            prosystem_extra_cycles = 0;
        }
        else {
            prosystem_extra_cycles = (prosystem_cycles % CYCLES_PER_SCANLINE);
            
            // Some fudge for Maria cycles. Unfortunately Maria cycle counting
            // isn't exact (This adds some extra cycles).
            prosystem_cycles = 0;
        }
        
        // If lightgun is enabled, check to see if it should be fired
        if (lightgun) prosystem_FireLightGun();
        
        while (prosystem_cycles < cartridge_hblank) {
            cycles = sally_ExecuteInstruction( );
            prosystem_cycles += (cycles << 2);
            
            if (half_cycle) prosystem_cycles += 2;
            
            if (riot_timing) {
                riot_UpdateTimer(cycles);
            }
            
            // If lightgun is enabled, check to see if it should be fired
            if (lightgun) prosystem_FireLightGun();
            
            if (memory_ram[WSYNC] && wsync) {
                memory_ram[WSYNC] = false;
                wsync_scanline = true;
                break;
            }
        }
        
        cycles = maria_RenderScanline( );
        
        if (cycle_stealing) {
            prosystem_cycles += cycles;
        
            if(riot_timing) {
                riot_UpdateTimer(cycles >> 2);
            }
        }
        
        while (!wsync_scanline && prosystem_cycles < CYCLES_PER_SCANLINE) {
            cycles = sally_ExecuteInstruction();
            prosystem_cycles += (cycles << 2);
            
            if(half_cycle) prosystem_cycles += 2;
            
            // If lightgun is enabled, check to see if it should be fired
            if (lightgun) prosystem_FireLightGun();
            
            if (riot_timing) {
                riot_UpdateTimer(cycles);
            }
            
            if(memory_ram[WSYNC] && wsync) {
                memory_ram[WSYNC] = false;
                wsync_scanline = true;
                break;
            }
        }
        
        // If a WSYNC was performed and the current cycle count is less than
        // the cycles per scanline, add those cycles to current timers.
        if (wsync_scanline && prosystem_cycles < CYCLES_PER_SCANLINE) {
            if (riot_timing) {
                riot_UpdateTimer((CYCLES_PER_SCANLINE - prosystem_cycles) >> 2);
            }
            prosystem_cycles = CYCLES_PER_SCANLINE;
        }
        
        // If lightgun is enabled, check to see if it should be fired
        if (lightgun) prosystem_FireLightGun();
        
        tia_Process(2);
        
        if (cartridge_pokey || cartridge_xm) {
            pokey_Process(2);
        }
        
        if (cartridge_pokey || cartridge_xm) pokey_Scanline();
    }
    
    prosystem_frame++;
    
    if (prosystem_frame >= prosystem_frequency) {
        prosystem_frame = 0;
    }
}

bool prosystem_Save(const char *filename) {
    uint8_t loc_buffer[33000 + XM_RAM_SIZE + 4] = {0};
    
    uint32_t size = 0;
    
    uint32_t index;
    
    for (index = 0; index < 16; index++) {
        loc_buffer[size + index] = PRO_SYSTEM_STATE_HEADER[index];
    }
    size += 16;
    
    loc_buffer[size++] = 1;
    for (index = 0; index < 4; index++) {
        loc_buffer[size + index] = 0;
    }
    size += 4;
    
    for (index = 0; index < 32; index++) {
        loc_buffer[size + index] = cart_digest[index];
    }
    size += 32;
    
    loc_buffer[size++] = sally_a;
    loc_buffer[size++] = sally_x;
    loc_buffer[size++] = sally_y;
    loc_buffer[size++] = sally_p;
    loc_buffer[size++] = sally_s;
    loc_buffer[size++] = sally_pc.b.l;
    loc_buffer[size++] = sally_pc.b.h;
    loc_buffer[size++] = cartridge_bank;
    
    for (index = 0; index < 16384; index++) {
        loc_buffer[size + index] = memory_ram[index];
    }
    size += 16384;
    
    if (cartridge_type == CARTRIDGE_TYPE_SUPERCART_RAM) {
        for(index = 0; index < 16384; index++) {
            loc_buffer[size + index] = memory_ram[16384 + index];
        }
        size += 16384;
    }
    
    // RIOT state
    loc_buffer[size++] = riot_dra;
    loc_buffer[size++] = riot_drb;
    loc_buffer[size++] = riot_timing;
    loc_buffer[size++] = ( 0xff & ( riot_timer >> 8 ) );
    loc_buffer[size++] = ( 0xff & riot_timer );
    loc_buffer[size++] = riot_intervals;
    loc_buffer[size++] = ( 0xff & ( riot_clocks >> 8 ) );
    loc_buffer[size++] = ( 0xff & riot_clocks );
    
    // XM (if applicable)
    if (cartridge_xm) {
        loc_buffer[size++] = xm_reg;
        loc_buffer[size++] = xm_bank;
        loc_buffer[size++] = xm_pokey_enabled;
        loc_buffer[size++] = xm_mem_enabled;
        
        for (index = 0; index < XM_RAM_SIZE; index++) {
            loc_buffer[size + index] = xm_ram[index];
        }
        size += XM_RAM_SIZE;
    }
    
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        return false;
    }
    
    if (fwrite(loc_buffer, 1, size, file) != size) {
        fclose(file);
        return false;
    }
    
    fflush(file);
    fclose(file);
    
    return true;
}

bool prosystem_Save_buffer(uint8_t *buffer) {
    uint32_t size = 0;
    uint32_t index;
    
    for (index = 0; index < 16; index++) {
        buffer[size + index] = PRO_SYSTEM_STATE_HEADER[index];
    }
    size += 16;
    
    buffer[size++] = 1;
    for (index = 0; index < 4; index++) {
        buffer[size + index] = 0;
    }
    size += 4;
    
    for (index = 0; index < 32; index++) {
        buffer[size + index] = cart_digest[index];
    }
    size += 32;
    
    buffer[size++] = sally_a;
    buffer[size++] = sally_x;
    buffer[size++] = sally_y;
    buffer[size++] = sally_p;
    buffer[size++] = sally_s;
    buffer[size++] = sally_pc.b.l;
    buffer[size++] = sally_pc.b.h;
    buffer[size++] = cartridge_bank;
    
    for (index = 0; index < 16384; index++) {
        buffer[size + index] = memory_ram[index];
    }
    size += 16384;
    
    if (cartridge_type == CARTRIDGE_TYPE_SUPERCART_RAM) {
        for (index = 0; index < 16384; index++) {
            buffer[size + index] = memory_ram[16384 + index];
        }
        size += 16384;
    }
    
    // RIOT state
    buffer[size++] = riot_dra;
    buffer[size++] = riot_drb;
    buffer[size++] = riot_timing;
    buffer[size++] = ( 0xff & ( riot_timer >> 8 ) );
    buffer[size++] = ( 0xff & riot_timer );
    buffer[size++] = riot_intervals;
    buffer[size++] = ( 0xff & ( riot_clocks >> 8 ) );
    buffer[size++] = ( 0xff & riot_clocks );
    
    // XM (if applicable)
    if (cartridge_xm) {
        buffer[size++] = xm_reg;
        buffer[size++] = xm_bank;
        buffer[size++] = xm_pokey_enabled;
        buffer[size++] = xm_mem_enabled;
        
        for (index = 0; index < XM_RAM_SIZE; index++) {
            buffer[size + index] = xm_ram[index];
        }
        size += XM_RAM_SIZE;
    }
    
    return true;
}

bool prosystem_Load(const char *filename) {
    uint8_t loc_buffer[33000 + XM_RAM_SIZE + 4] = {0};
    
    uint32_t size = 0;
    if (size == 0) {
        FILE* file = fopen(filename, "rb");
        if (file == NULL) {
            return false;
        }
        
        if (fseek(file, 0L, SEEK_END)) {
            fclose(file);
            return false;
        }
        
        size = ftell(file);
        if (fseek(file, 0L, SEEK_SET)) {
            fclose(file);
            return false;
        }
        
        if (size != 16445 && size != 32829 &&     /* no RIOT */
            size != 16453 && size != 32837 &&     /* with RIOT */
            size != (16453 + 4 + XM_RAM_SIZE) &&  /* XM without supercart ram */
            size != (32837 + 4 + XM_RAM_SIZE))    /* XM with supercart ram */
            {
            fclose(file);
            return false;
        }
        
        if (fread(loc_buffer, 1, size, file) != size && ferror(file)) {
            fclose(file);
            return false;
        }
        fclose(file);
    }
    
    uint32_t offset = 0;
    uint32_t index;
    for (index = 0; index < 16; index++) {
        if (loc_buffer[offset + index] != PRO_SYSTEM_STATE_HEADER[index]) {
            return false;
        }
    }
    offset += 16;
    //uint8_t version = loc_buffer[offset++];
    offset++;
    
    for (index = 0; index < 4; index++) {
    }
    offset += 4;
    
    prosystem_Reset();
    
    char digest[33] = {0};
    for (index = 0; index < 32; index++) {
        digest[index] = loc_buffer[offset + index];
    }
    offset += 32;
    
    if (strcmp(cart_digest, digest)) { // Not the same
        return false;
    }
    
    sally_a = loc_buffer[offset++];
    sally_x = loc_buffer[offset++];
    sally_y = loc_buffer[offset++];
    sally_p = loc_buffer[offset++];
    sally_s = loc_buffer[offset++];
    sally_pc.b.l = loc_buffer[offset++];
    sally_pc.b.h = loc_buffer[offset++];
    
    cartridge_StoreBank(loc_buffer[offset++]);
    
    for(index = 0; index < 16384; index++) {
        memory_ram[index] = loc_buffer[offset + index];
    }
    offset += 16384;
    
    if (cartridge_type == CARTRIDGE_TYPE_SUPERCART_RAM) {
        if (size != 32829 && /* no RIOT */
            size != 32837 && /* with RIOT */
            size != (32837 + 4 + XM_RAM_SIZE)) /* XM */ {
            return false;
        }
        
        for (index = 0; index < 16384; index++) {
            memory_ram[16384 + index] = loc_buffer[offset + index];
        }
        offset += 16384;
    }
    
    if (size == 16453 || /* no supercart ram */
        size == 32837 || /* supercart ram */
        size == (16453 + 4 + XM_RAM_SIZE) || /* xm, no supercart ram */
        size == (32837 + 4 + XM_RAM_SIZE)) /* xm, supercart ram */ {
        // RIOT state
        riot_dra = loc_buffer[offset++];
        riot_drb = loc_buffer[offset++];
        riot_timing = loc_buffer[offset++];
        riot_timer = ( loc_buffer[offset++] << 8 );
        riot_timer |= loc_buffer[offset++];
        riot_intervals = loc_buffer[offset++];
        riot_clocks = ( loc_buffer[offset++] << 8 );
        riot_clocks |= loc_buffer[offset++];
    }
    
    // XM (if applicable)
    if (cartridge_xm) {
        if ((size != (16453 + 4 + XM_RAM_SIZE)) &&
            (size != (32837 + 4 + XM_RAM_SIZE))) {
            return false; // Save state file has an invalid size
        }
        xm_reg = loc_buffer[offset++];
        xm_bank = loc_buffer[offset++];
        xm_pokey_enabled = loc_buffer[offset++];
        xm_mem_enabled = loc_buffer[offset++];
        
        for (index = 0; index < XM_RAM_SIZE; index++) {
            xm_ram[index] = loc_buffer[offset++];
        }
    }
    
    return true;
}

bool prosystem_Load_buffer(const uint8_t *buffer) {
    uint32_t size = cartridge_type == CARTRIDGE_TYPE_SUPERCART_RAM ? 32837 : 16453;
    if(cartridge_xm) {
        size += 4 + XM_RAM_SIZE;
    }
    uint32_t offset = 0;
    uint32_t index;
    
    for (index = 0; index < 16; index++) {
        if (buffer[offset + index] != PRO_SYSTEM_STATE_HEADER[index]) {
            return false;
        }
    }
    offset += 16;
    
    //uint8_t version = buffer[offset++];
    offset++;
    //uint32_t date = 0;
    
    offset += 4;
    
    //prosystem_Reset(); // TODO doesn't seem necessary but needs investigation
    
    char digest[33] = {0};
    for (index = 0; index < 32; index++) {
        digest[index] = buffer[offset + index];
    }
    offset += 32;
    
    if (strcmp(cart_digest, digest)) { // Not the same
        return false;
    }
    
    sally_a = buffer[offset++];
    sally_x = buffer[offset++];
    sally_y = buffer[offset++];
    sally_p = buffer[offset++];
    sally_s = buffer[offset++];
    sally_pc.b.l = buffer[offset++];
    sally_pc.b.h = buffer[offset++];
    
    cartridge_StoreBank(buffer[offset++]);
    
    for (index = 0; index < 16384; index++) {
        memory_ram[index] = buffer[offset + index];
    }
    offset += 16384;
    
    if (cartridge_type == CARTRIDGE_TYPE_SUPERCART_RAM) {
        if (size != 32829 && /* no RIOT */
            size != 32837 && /* with RIOT */
            size != (32837 + 4 + XM_RAM_SIZE)) /* XM */ {
            return false; // Save state file has an invalid size.
        }
        for (index = 0; index < 16384; index++) {
            memory_ram[16384 + index] = buffer[offset + index];
        }
        offset += 16384;
    }
    
    if (size == 16453 || /* no supercart ram */
        size == 32837 || /* supercart ram */
        size == (16453 + 4 + XM_RAM_SIZE) || /* xm, no supercart ram */
        size == (32837 + 4 + XM_RAM_SIZE)) /* xm, supercart ram */ {
        // RIOT state
        riot_dra = buffer[offset++];
        riot_drb = buffer[offset++];
        riot_timing = buffer[offset++];
        riot_timer = ( buffer[offset++] << 8 );
        riot_timer |= buffer[offset++];
        riot_intervals = buffer[offset++];
        riot_clocks = ( buffer[offset++] << 8 );
        riot_clocks |= buffer[offset++];
    }
    
    // XM (if applicable)
    if (cartridge_xm) {
        if ((size != (16453 + 4 + XM_RAM_SIZE)) &&
            (size != (32837 + 4 + XM_RAM_SIZE))) {
            return false; // Save state file has an invalid size.
        }
        xm_reg = buffer[offset++];
        xm_bank = buffer[offset++];
        xm_pokey_enabled = buffer[offset++];
        xm_mem_enabled = buffer[offset++];
        
        for (index = 0; index < XM_RAM_SIZE; index++) {
            xm_ram[index] = buffer[offset++];
        }
    }
    
    return true;
}

void prosystem_Pause(bool pause) {
    if (prosystem_active) {
        prosystem_paused = pause;
    }
}

void prosystem_Close(void) {
    prosystem_active = false;
    prosystem_paused = false;
    cartridge_Release();
    maria_Reset();
    maria_Clear();
    memory_Reset();
    tia_Reset();
    tia_Clear();
}
