/*
    This file is part of CrabEmu.

    Copyright (C) 2015 Lawrence Sebald

    CrabEmu is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    CrabEmu is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CrabEmu; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#ifdef DEBUG
#include <inttypes.h>
#endif

#ifdef _arch_dreamcast
#include <zlib/zlib.h>
#include <bzlib/bzlib.h>
#else
#ifndef NO_ZLIB
#include <zlib.h>
#endif

#ifndef NO_BZ2
#include <bzlib.h>
#endif
#endif

#include "rom.h"
#include "chip8.h"
#include "chip8cpu.h"
#include "sound.h"

#ifndef NO_ZLIB
#include "unzip.h"
#endif

/* Forward declaration... */
chip8_t chip8_cons;

static uint8 *cart_rom;
static uint32 cart_len;
static uint8 memory[4096];
static uint32 rom_crc, rom_adler;
static pixel_t fb[64 * 32];
static Chip8CPU_t cpu;
static uint8 buttons[16];
static uint8 dt, st;

static const uint8 font[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80
};

#define PIXEL_SET ((pixel_t)(0xFFFFFFFF))

static uint8 chip8_mem_read(void *cpu, uint16 addr) {
    (void)cpu;
    return memory[addr & 0xFFF];
}

static void chip8_mem_write(void *cpu, uint16 addr, uint8 val) {
    (void)cpu;
    memory[addr & 0xFFF] = val;
}

static uint16 chip8_mem_read16(void *cpu, uint16 addr) {
    (void)cpu;
    return (memory[addr & 0xFFF] << 8) | memory[(addr + 1) & 0xFFF];
}

static uint8 chip8_reg_read(void *cpu, uint8 reg) {
    (void)cpu;

    if(reg < CHIP8_REG_DT)
        return buttons[reg];
    else if(reg == CHIP8_REG_DT)
        return dt;
    else if(reg == CHIP8_REG_ST)
        return st;
    else
        return 0;
}

static void chip8_reg_write(void *cpu, uint8 reg, uint8 val) {
    (void)cpu;

    if(reg == CHIP8_REG_DT)
        dt = val;
    else if(reg == CHIP8_REG_ST)
        st = val;
}

static void chip8_mem_clear_fb(void *cpu) {
    (void)cpu;
    memset(fb, 0, 64 * 32 * sizeof(pixel_t));
}

static int chip8_mem_draw_spr(void *cpu, uint16 addr, uint8 x, uint8 y,
                              uint8 height) {
    uint8 i, j, bp;
    int rv = 0;
    int ptr;

    y &= 0x1F;
    x &= 0x3F;
    ptr = (y << 6) + x;

    if(height + y > 32)
        height = 32 - y;

    for(i = 0; i < height; ++i, ptr += 64) {
        bp = chip8_mem_read(cpu, addr + i);

        for(j = 0; j < 8 && x + j < 64; ++j) {
            if((bp & (0x80 >> j))) {
                fb[ptr + j] ^= PIXEL_SET;
                if(!fb[ptr + j])
                    rv = 1;
            }
        }
    }

    return rv;
}

static void finalize_load(const char *fn) {
    char *name;

    /* Calculate the checksums of the game */
    rom_adler = rom_adler32(cart_rom, cart_len);
    rom_crc = rom_crc32(cart_rom, cart_len);

#ifdef DEBUG
    printf("Checksums: Adler-32: 0x%08" PRIX32 " CRC32: 0x%08" PRIX32 "\n",
           (uint32_t)rom_adler, (uint32_t)rom_crc);
#endif

    /* Copy the rom and font into the memory space. */
    memcpy(memory + 0x200, cart_rom, cart_len);
    memcpy(memory, font, 80);
    chip8_mem_clear_fb(NULL);

    name = strrchr(fn, '/');

    if(name != NULL) {
        /* we have a filename, but the first char is /, so skip that */
        gui_set_title(name + 1);
    }
    else {
        gui_set_title("CrabEmu");
    }
}

#ifndef NO_ZLIB
static int load_gz_rom(const char *fn) {
    int len;
    gzFile fp;
    void *tmp;

    /* Open up the file in question. */
    if(!(fp = gzopen(fn, "rb"))) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_OPEN;
    }

    /* Read one chunk of the file at a time, until we have no more left. */
    cart_rom = (uint8 *)malloc(0x100);

    if(!cart_rom) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Couldn't allocate space!\n");
#endif
        gzclose(fp);
        return ROM_LOAD_E_ERRNO;
    }

    cart_len = 0;
    len = gzread(fp, cart_rom, 0x100);

    while(len != 0 && len != -1) {
        cart_len += len;
        tmp = realloc(cart_rom, cart_len + 0x100);

        if(!tmp) {
#ifdef DEBUG
            fprintf(stderr, "chip8_mem_load_rom: Couldn't allocate space!\n");
#endif
            gzclose(fp);
            free(cart_rom);
            cart_rom = NULL;
            return ROM_LOAD_E_ERRNO;
        }

        cart_rom = (uint8 *)tmp;
        memset(cart_rom + cart_len, 0xFF, 0x100);
        len = gzread(fp, cart_rom + cart_len, 0x100);
    }

    gzclose(fp);
    finalize_load(fn);

    return ROM_LOAD_SUCCESS;
}
#endif

#ifndef NO_BZ2
static int load_bz2_rom(const char *fn) {
    int len;
    BZFILE *fp;
    void *tmp;

    /* Open up the file in question. */
    if(!(fp = BZ2_bzopen(fn, "rb"))) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_OPEN;
    }

    /* Read one chunk of the file at a time, until we have no more left. */
    if(!(cart_rom = (uint8 *)malloc(0x100))) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Couldn't allocate space!\n");
#endif
        BZ2_bzclose(fp);
        return ROM_LOAD_E_ERRNO;
    }

    cart_len = 0;
    len = BZ2_bzread(fp, cart_rom, 0x100);

    while(len != 0 && len != -1) {
        cart_len += len;
        tmp = realloc(cart_rom, cart_len + 0x100);

        if(!tmp) {
#ifdef DEBUG
            fprintf(stderr, "chip8_mem_load_rom: Couldn't allocate space!\n");
#endif
            BZ2_bzclose(fp);
            free(cart_rom);
            cart_rom = NULL;
            return ROM_LOAD_E_ERRNO;
        }

        cart_rom = (uint8 *)tmp;
        memset(cart_rom + cart_len, 0xFF, 0x100);
        len = BZ2_bzread(fp, cart_rom + cart_len, 0x100);
    }

    BZ2_bzclose(fp);
    finalize_load(fn);

    return ROM_LOAD_SUCCESS;
}
#endif

#ifndef NO_ZLIB
static int load_zip_rom(const char *fn) {
    int found = 0;
    unzFile fp;
    unz_file_info inf;
    char fn2[4096];
    char *ext;
    int size;

    /* Open up the file in question. */
    fp = unzOpen(fn);

    if(!fp) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_OPEN;
    }

    if(unzGoToFirstFile(fp) != UNZ_OK) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Couldn't find file in zip!\n");
#endif
        unzClose(fp);
        return ROM_LOAD_E_CORRUPT;
    }

    /* Keep looking at files until we find a rom. */
    while(!found) {
        if(unzGetCurrentFileInfo(fp, &inf, fn2, 4096, NULL, 0, NULL,
                                 0) != UNZ_OK) {
#ifdef DEBUG
            fprintf(stderr, "chip8_mem_load_rom: Error parsing Zip file!\n");
#endif
            unzClose(fp);
            return ROM_LOAD_E_CORRUPT;
        }

        /* Check if this file looks like a rom. */
        ext = strrchr(fn2, '.');

        if(ext) {
            /* XXXX */
            if(!strcmp(ext, ".ch8") || !strcmp(ext, ".c8")) {
                found = 1;
            }
        }

        /* If we haven't found a rom yet, move to the next file. */
        if(!found) {
            if(unzGoToNextFile(fp) != UNZ_OK) {
#ifdef DEBUG
                fprintf(stderr, "chip8_mem_load_rom: End of zip file!\n");
#endif
                unzClose(fp);
                return ROM_LOAD_E_NO_ROM;
            }
        }
    }

    /* If the size isn't a nice even amount, round it. */
    size = inf.uncompressed_size;

    if(!(cart_rom = (uint8 *)malloc(size))) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Could not load ROM: %s!\n", fn);
        fprintf(stderr, "Reason: %s\n", strerror(errno));
#endif
        unzClose(fp);
        return ROM_LOAD_E_ERRNO;
    }

    /* Attempt to open the file we think is a rom. */
    if(unzOpenCurrentFile(fp) != UNZ_OK) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Couldn't open rom from zip!\n");
#endif
        unzClose(fp);
        free(cart_rom);
        cart_rom = NULL;
        return ROM_LOAD_E_CORRUPT;
    }

    /* Attempt to read the file. */
    if(unzReadCurrentFile(fp, cart_rom, size) < 0) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Couldn't read rom from zip!\n");
#endif
        unzCloseCurrentFile(fp);
        unzClose(fp);
        free(cart_rom);
        cart_rom = NULL;
        return ROM_LOAD_E_CORRUPT;
    }

    /* We're done with the file, close everything. */
    unzCloseCurrentFile(fp);
    unzClose(fp);

    cart_len = size;
    finalize_load(fn);

    return ROM_LOAD_SUCCESS;
}
#endif

int chip8_mem_load_rom(const char *fn) {
    FILE *fp;
    int size;
    char *ext;

    /* If its a compressed format, go onto the handler for it... */
    ext = strrchr(fn, '.');

    if(ext) {
#ifndef NO_ZLIB
        if(!strcmp(ext, ".gz")) {
            return load_gz_rom(fn);
        }
        else if(!strcmp(ext, ".zip")) {
            return load_zip_rom(fn);
        }
#endif
#ifndef NO_BZ2
        if(!strcmp(ext, ".bz2")) {
            return load_bz2_rom(fn);
        }
#endif
    }

    if(!(fp = fopen(fn, "rb"))) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_ERRNO;
    }

    /* Determine the size of the file */
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if(!(cart_rom = (uint8 *)malloc(size))) {
#ifdef DEBUG
        fprintf(stderr, "chip8_mem_load_rom: Could not load ROM: %s!\n", fn);
        fprintf(stderr, "Reason: %s\n", strerror(errno));
#endif
        return ROM_LOAD_E_ERRNO;
    }

    fread(cart_rom, size, 1, fp);
    fclose(fp);

    cart_len = size;
    finalize_load(fn);

    return ROM_LOAD_SUCCESS;
}

static int chip8_mem_init(void) {
    memset(memory, 0, 0x1000);
    chip8_mem_clear_fb(NULL);

    return 0;
}

static int chip8_mem_shutdown(void) {
    if(cart_rom)
        free(cart_rom);

    cart_rom = NULL;
    cart_len = 0;

    return 0;
}

static void chip8_mem_reset(void) {
    memset(memory, 0, 0x1000);
    memcpy(memory + 0x200, cart_rom, cart_len);
    memcpy(memory, font, 80);
    chip8_mem_clear_fb(NULL);
}

void chip8_get_checksums(uint32 *crc, uint32 *adler) {
    *crc = rom_crc;
    *adler = rom_adler;
}

/* Console interface for CrabEmu's core... */
int chip8_init(void) {
    if(chip8_cons._base.initialized)
        return -1;

    gui_set_console((console_t *)&chip8_cons);

    Chip8CPU_init(&cpu);
    Chip8CPU_set_memread(&cpu, &chip8_mem_read);
    Chip8CPU_set_memread16(&cpu, &chip8_mem_read16);
    Chip8CPU_set_memwrite(&cpu, &chip8_mem_write);
    Chip8CPU_set_regread(&cpu, &chip8_reg_read);
    Chip8CPU_set_regwrite(&cpu, &chip8_reg_write);
    Chip8CPU_set_clearfb(&cpu, &chip8_mem_clear_fb);
    Chip8CPU_set_drawspr(&cpu, &chip8_mem_draw_spr);

    chip8_mem_init();

    sound_init(1, VIDEO_NTSC);

    chip8_cons._base.initialized = 1;

    return 0;
}

static int chip8_shutdown(void) {
    if(!chip8_cons._base.initialized)
        return -1;

    chip8_mem_clear_fb(NULL);
    Chip8CPU_reset(&cpu);
    chip8_mem_shutdown();
    sound_shutdown();

    chip8_cons._base.initialized = 0;

    return 0;
}

static int chip8_reset(void) {
    if(!chip8_cons._base.initialized)
        return -1;

    sound_reset_buffer();
    chip8_mem_shutdown();
    chip8_mem_init();
    Chip8CPU_reset(&cpu);

    return 0;
}

static int chip8_soft_reset(void) {
    if(!chip8_cons._base.initialized)
        return 0;

    sound_reset_buffer();
    chip8_mem_reset();
    Chip8CPU_reset(&cpu);

    return 0;
}

static void chip8_frame(int skip) {
    int i;

    (void)skip;

    /* XXXX: How many cycles to execute? We punt and do 15. */
    for(i = 0; i < 15; ++i) {
        Chip8CPU_execute(&cpu);
    }

    /* Update the timers, if they're running. */
    if(dt)
        --dt;

    if(st) {
        /* Need to deal with sound still... */
        --st;
    }
}

static void chip8_button_pressed(int player, int button) {
    if(player != 1)
        return;

    if(button < 0 || button > 15)
        return;

    buttons[button] = 1;
}

static void chip8_button_released(int player, int button) {
    if(player != 1)
        return;

    if(button < 0 || button > 15)
        return;

    buttons[button] = 0;
}

static void *chip8_framebuffer(void) {
    return fb;
}

static void chip8_framesize(uint32_t *x, uint32_t *y) {
    *x = 64;
    *y = 32;
}

static void chip8_activeframe(uint32_t *x, uint32_t *y, uint32_t *w,
                              uint32_t *h) {
    *x = *y = 0;
    *w = 64;
    *h = 32;
}

/* Console declaration... */
chip8_t chip8_cons = {
    {
        CONSOLE_CHIP8,
        CONSOLE_CHIP8,
        0,
        &chip8_shutdown,
        &chip8_reset,
        &chip8_soft_reset,
        &chip8_frame,
        NULL,
        NULL,
        NULL,
        &chip8_button_pressed,
        &chip8_button_released,
        &chip8_framebuffer,
        &chip8_framesize,
        &chip8_activeframe,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    }
};
