// ----------------------------------------------------------------------------
//   ___  ___  ___  ___       ___  ____  ___  _  _      __  ___
//  /__/ /__/ /  / /__  /__/ /__    /   /_   / |/ /      / / _
// /    / \  /__/ ___/ ___/ ___/   /   /__  /    /   ___/ /__/
//
// ----------------------------------------------------------------------------
// Copyright 2005 Greg Stanton
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
// Cartridge.c
// ----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "Cartridge.h"

char cart_digest[33];

const char *cartridge_filename;
char cartridge_title[33];
uint8_t cartridge_type;
uint8_t cartridge_region;
bool cartridge_pokey;
bool cartridge_pokey450;
uint8_t cartridge_controller[2];
uint8_t cartridge_bank;
uint32_t cartridge_flags;
int cartridge_crosshair_x;
int cartridge_crosshair_y;
bool cartridge_dualanalog = false;
bool cartridge_xm = false;
bool cartridge_disable_bios = false;
uint8_t cartridge_left_switch = 1;
uint8_t cartridge_right_switch = 0;
bool cartridge_swap_buttons = false;
bool cartridge_hsc_enabled = false;
uint32_t cartridge_hblank = HBLANK_DEFAULT;

static uint8_t *cartridge_buffer = NULL;
static size_t cartridge_size = 0;

static bool cartridge_HasHeader(const uint8_t* header) {
    const char HEADER_ID[ ] = {"ATARI7800"};
    
    for (int index = 0; index < 9; index++) {
        if (HEADER_ID[index] != header[index + 1]) {
            return false;
        }
    }
    return true;
}

static bool cartridge_CC2(const uint8_t* header) {
    const char HEADER_ID[ ] = {">>"};
    for (int index = 0; index < 2; index++) {
        if (HEADER_ID[index] != header[index+1]) {
            return false;
        }
    }
    return true;
}

static uint32_t cartridge_GetBank(uint8_t bank) {
    if ((cartridge_type == CARTRIDGE_TYPE_SUPERCART ||
        cartridge_type == CARTRIDGE_TYPE_SUPERCART_ROM ||
        cartridge_type == CARTRIDGE_TYPE_SUPERCART_RAM) &&
        cartridge_size <= 65536) {
        // for some of these carts, there are only 4 banks. in this case we
        // ignore bit 3 previously, games of this type had to be doubled. The
        // first 4 banks needed to be duplicated at the end of the ROM
        return (bank & 3);
    }
    
    return bank;
}

static uint32_t cartridge_GetBankOffset(uint8_t bank) {
    return cartridge_GetBank(bank) * 16384;
}

static void cartridge_WriteBank(uint16_t address, uint8_t bank) {
    uint32_t offset = cartridge_GetBankOffset(bank);
    if (offset < cartridge_size) {
        memory_WriteROM(address, 16384, cartridge_buffer + offset);
        cartridge_bank = bank;
    }
}

static void cartridge_SetTypeBySize(uint32_t size) {
    if (size <= 0x10000) {
        cartridge_type = CARTRIDGE_TYPE_NORMAL;
    }
    else if (size == 0x24000) {
        cartridge_type = CARTRIDGE_TYPE_SUPERCART_LARGE;
    }
    else if (size == 0x20000) {
        cartridge_type = CARTRIDGE_TYPE_SUPERCART_ROM;
    }
    else {
        cartridge_type = CARTRIDGE_TYPE_SUPERCART;
    }
}

static void cartridge_ReadHeader(const uint8_t* header) {
    for (int index = 0; index < 32; index++) {
        cartridge_title[index] = header[index + 17];
    }
    
    cartridge_size  = header[49] << 24;
    cartridge_size |= header[50] << 16;
    cartridge_size |= header[51] << 8;
    cartridge_size |= header[52];
    
    if (header[53] == 0) {
        if (cartridge_size > 131072) {
            cartridge_type = CARTRIDGE_TYPE_SUPERCART_LARGE;
        }
        else if (header[54] == 2 || header[54] == 3) {
            cartridge_type = CARTRIDGE_TYPE_SUPERCART;
        }
        else if (header[54] == 4 || header[54] == 5 || header[54] == 6 ||
            header[54] == 7) {
            cartridge_type = CARTRIDGE_TYPE_SUPERCART_RAM;
        }
        else if (header[54] == 8 || header[54] == 9 || header[54] == 10 ||
            header[54] == 11) {
            cartridge_type = CARTRIDGE_TYPE_SUPERCART_ROM;
        }
        else {
            cartridge_type = CARTRIDGE_TYPE_NORMAL;
        }
    }
    else {
        if (header[53] == 2 /*1*/) { // Wii: Abs and Act were swapped
            cartridge_type = CARTRIDGE_TYPE_ABSOLUTE;
        }
        else if (header[53] == 1 /*2*/) { // Wii: Abs and Act were swapped
            cartridge_type = CARTRIDGE_TYPE_ACTIVISION;
        }
        else {
            cartridge_type = CARTRIDGE_TYPE_NORMAL;
        }
    }
    
    cartridge_pokey = (header[54] & 1) ? true : false;
    cartridge_pokey450 = (header[54] & 0x40)? true : false;
    if (cartridge_pokey450) {
        cartridge_pokey = true;
    }
    cartridge_controller[0] = header[55];
    cartridge_controller[1] = header[56];
    cartridge_region = header[57];
    cartridge_flags = 0;
    cartridge_xm = (header[63] & 1) ? true: false;
    cartridge_hsc_enabled = header[58] & 0x01;
    
      // Wii: Updates to header interpretation
    uint8_t ct1 = header[54];
    if (header[53] == 0) {
        // BIT1 and BIT3 (Supercart Large: 2) rom at $4000
        if ((ct1 & 0x0a) == 0x0a) {
            cartridge_type = CARTRIDGE_TYPE_SUPERCART_LARGE;
        }
        // BIT1 and BIT4 (Supercart ROM: 4) bank6 at $4000
        else if ((ct1 & 0x12) == 0x12) {
            cartridge_type = CARTRIDGE_TYPE_SUPERCART_ROM;
        }
        // BIT1 and BIT2 (Supercart RAM: 3) ram at $4000
        else if ((ct1 & 0x06) == 0x06) {
            cartridge_type = CARTRIDGE_TYPE_SUPERCART_RAM;
        }
        // BIT1 (Supercart) bank switched
        else if ((ct1 & 0x02) == 0x02) {
            cartridge_type = CARTRIDGE_TYPE_SUPERCART;
        }
        // Size < 64k && BIT2 (Normal RAM: ?) ram at $4000 )
        else if (cartridge_size <= 0x10000 && ((ct1&0x04)==0x04)) {
            cartridge_type = CARTRIDGE_TYPE_NORMAL_RAM;
        }
        // Attempt to determine the cartridge type based on its size
        else {
            cartridge_SetTypeBySize(cartridge_size);
        }
    }
}

// Convert a nybble's hexadecimal representation to a lower case ASCII char
static inline char nyb_hexchar(unsigned nyb) {
    nyb &= 0xf; // Lower nybble only
    if (nyb >= 10)
        nyb += 'a' - 10;
    else
        nyb += '0';
    return (char)nyb;
}

bool cartridge_Load(const uint8_t* data, uint32_t size) {
    if (size <= 128) {
        // Cartridge data is invalid.
        return false;
    }
    
    cartridge_Release( );
    
    uint8_t header[128] = {0};
    
    for(int index = 0; index < 128; index++) {
        header[index] = data[index];
    }
    
    if (cartridge_CC2(header)) {
        // Prosystem doesn't support CC2 hacks.
        return false;
    }
    
    uint32_t offset = 0;
    
    if (cartridge_HasHeader(header)) {
        cartridge_ReadHeader(header);
        size -= 128;
        offset = 128;
        
        // Several cartridge headers do not have the proper size. So attempt to
        // use the size of the file.
        if (cartridge_size != size) {
            // Necessary for the following roms:
            // Impossible Mission hacks w/ C64 style graphics
            if (size % 1024 == 0) {
                cartridge_size = size;
            }
        }
    }
    else {
        cartridge_size = size;
        // Attempt to guess the cartridge type based on its size
        cartridge_SetTypeBySize(size);
    }
    
    cartridge_buffer = (uint8_t*)malloc(cartridge_size * sizeof(uint8_t));
    
    for(size_t index = 0; index < cartridge_size; index++) {
        cartridge_buffer[index] = data[index + offset];
    }
    
    // Different from the file md5sum which starts from the header vs rom data
    MD5_CTX c;
    size_t md5len = cartridge_size;
    uint8_t *dataptr = cartridge_buffer;
    unsigned char digest[16];
    MD5_Init(&c);
    MD5_Update(&c, dataptr, md5len);
    MD5_Final(digest, &c);
    /*for (int i = 0; i < 16; i++) {
        snprintf(&(cart_digest[i * 2]), 16 * 2, "%02x", (unsigned)digest[i]);
    }*/

    // Convert the digest to a string without dodgy calls to snprintf
    for (int i = 0; i < 16; ++i) {
        cart_digest[i * 2] = nyb_hexchar(digest[i] >> 4);
        cart_digest[(i * 2) + 1] = nyb_hexchar(digest[i]);
    }
    cart_digest[32] = '\0';
    
    return true;
}

void cartridge_Store(void) {
    switch(cartridge_type) {
        case CARTRIDGE_TYPE_NORMAL: {
            memory_WriteROM(65536 - cartridge_size, cartridge_size, cartridge_buffer);
            break;
        }
        case CARTRIDGE_TYPE_NORMAL_RAM: {
            memory_WriteROM(65536 - cartridge_size, cartridge_size, cartridge_buffer);
            memory_ClearROM(16384, 16384);
            break;
        }
        case CARTRIDGE_TYPE_SUPERCART: {
            uint32_t offset = cartridge_size - 16384;
            if (offset < cartridge_size) {
                memory_WriteROM(49152, 16384, cartridge_buffer + offset);
            }
            break;
        }
        case CARTRIDGE_TYPE_SUPERCART_LARGE: {
            uint32_t offset = cartridge_size - 16384;
            if (offset < cartridge_size) {
                memory_WriteROM(49152, 16384, cartridge_buffer + offset);
                memory_WriteROM(16384, 16384, cartridge_buffer + cartridge_GetBankOffset(0));
            }
            break;
        }
        case CARTRIDGE_TYPE_SUPERCART_RAM: {
            uint32_t offset = cartridge_size - 16384;
            if (offset < cartridge_size) {
                memory_WriteROM(49152, 16384, cartridge_buffer + offset);
                memory_ClearROM(16384, 16384);
            }
            break;
        }
        case CARTRIDGE_TYPE_SUPERCART_ROM: {
            uint32_t offset = cartridge_size - 16384;
            if (offset < cartridge_size && cartridge_GetBankOffset(6) < cartridge_size) {
                memory_WriteROM(49152, 16384, cartridge_buffer + offset);
                memory_WriteROM(16384, 16384, cartridge_buffer + cartridge_GetBankOffset(6));
            }
            break;
        }
        case CARTRIDGE_TYPE_ABSOLUTE: {
            memory_WriteROM(16384, 16384, cartridge_buffer);
            memory_WriteROM(32768, 32768, cartridge_buffer + cartridge_GetBankOffset(2));
            break;
        }
        case CARTRIDGE_TYPE_ACTIVISION: {
            if (122880 < cartridge_size) {
                memory_WriteROM(40960, 16384, cartridge_buffer);
                memory_WriteROM(16384, 8192, cartridge_buffer + 106496);
                memory_WriteROM(24576, 8192, cartridge_buffer + 98304);
                memory_WriteROM(32768, 8192, cartridge_buffer + 122880);
                memory_WriteROM(57344, 8192, cartridge_buffer + 114688);
            }
            break;
        }
    }
}

void cartridge_Write(uint16_t address, uint8_t data) {
    switch(cartridge_type) {
        case CARTRIDGE_TYPE_SUPERCART:
        case CARTRIDGE_TYPE_SUPERCART_RAM:
        case CARTRIDGE_TYPE_SUPERCART_ROM:
            if (address >= 32768 && address < 49152 && data < 9) {
                cartridge_StoreBank(data);
            }
            break;
        
        case CARTRIDGE_TYPE_SUPERCART_LARGE:
            if (address >= 32768 && address < 49152 && data < 9) {
                cartridge_StoreBank(data + 1);
            }
            break;
        
        case CARTRIDGE_TYPE_ABSOLUTE:
            if (address == 32768 && (data == 1 || data == 2)) {
                cartridge_StoreBank(data - 1);
            }
            break;
        
        case CARTRIDGE_TYPE_ACTIVISION:
            if (address >= 65408) {
                cartridge_StoreBank(address & 7);
            }
            break;
    }
#if 0 // Wii: Moved to Memory.cpp
    if (cartridge_pokey && address >= 0x4000 && address <= 0x400f) {
        switch (address) {
            case POKEY_AUDF1:
                pokey_SetRegister(POKEY_AUDF1, data);
                break;
            
            case POKEY_AUDC1:
                pokey_SetRegister(POKEY_AUDC1, data);
                break;
            
            case POKEY_AUDF2:
                pokey_SetRegister(POKEY_AUDF2, data);
                break;
            
            case POKEY_AUDC2:
                pokey_SetRegister(POKEY_AUDC2, data);
                break;
            
            case POKEY_AUDF3:
                pokey_SetRegister(POKEY_AUDF3, data);
                break;
            
            case POKEY_AUDC3:
                pokey_SetRegister(POKEY_AUDC3, data);
                break;
            
            case POKEY_AUDF4:
                pokey_SetRegister(POKEY_AUDF4, data);
                break;
            
            case POKEY_AUDC4:
                pokey_SetRegister(POKEY_AUDC4, data);
                break;
            
            case POKEY_AUDCTL:
                pokey_SetRegister(POKEY_AUDCTL, data);
                break;
            
            case POKEY_SKCTLS:
                pokey_SetRegister(POKEY_SKCTLS, data);
                break;
        }
    }
#endif
}

void cartridge_StoreBank(uint8_t bank) {
    switch (cartridge_type) {
        case CARTRIDGE_TYPE_SUPERCART:
            cartridge_WriteBank(32768, bank);
            break;
        
        case CARTRIDGE_TYPE_SUPERCART_RAM:
            cartridge_WriteBank(32768, bank);
            break;
        
        case CARTRIDGE_TYPE_SUPERCART_ROM:
            cartridge_WriteBank(32768, bank);
            break;
        
        case CARTRIDGE_TYPE_SUPERCART_LARGE:
            cartridge_WriteBank(32768, bank);
            break;
        
        case CARTRIDGE_TYPE_ABSOLUTE:
            cartridge_WriteBank(16384, bank);
            break;
        
        case CARTRIDGE_TYPE_ACTIVISION:
            cartridge_WriteBank(40960, bank);
            break;
    }
}

bool cartridge_IsLoaded(void) {
    return (cartridge_buffer != NULL) ? true : false;
}

void cartridge_Release(void) {
    if(cartridge_buffer != NULL) {
        free(cartridge_buffer);
        cartridge_size = 0;
        cartridge_buffer = NULL;
        cartridge_title[0] = '\0';
        cartridge_type = 0;
        cartridge_region = 0;
        cartridge_pokey = 0;
        cartridge_pokey450 = 0;
        cartridge_xm = false;
        // Default to joysticks
        memset(cartridge_controller, 1, sizeof(cartridge_controller));
        cartridge_bank = 0;
        cartridge_flags = 0;
        cartridge_disable_bios = false;
        cartridge_crosshair_x = 0;
        cartridge_crosshair_y = 0;
        cartridge_hblank = HBLANK_DEFAULT;
        cartridge_dualanalog = false;
        cartridge_left_switch = 1;
        cartridge_right_switch = 0;
        cartridge_swap_buttons = false;
        cartridge_hsc_enabled = false;
    }
}
