/*
    This file is part of CrabEmu.

    Copyright (C) 2012, 2013 Lawrence Sebald

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

#include "nesmem.h"
#include "nesppu.h"

/* iNES Mapper 0: NROM

   This "mapper" is really the lack of a mapper entirely. There is no swapping
   or anything else involved. */

static void init(void) {
    int is_rom = 1, i;
    int banks = nes_prg_rom_size >> 8;

    /* Fill in the read map */
    for(i = 0; i < 0x80; ++i) {
        nes_read_map[i + 0x80] = nes_prg_rom + ((i & (banks - 1)) << 8);
    }

    for(i = 0; i < 0x20; ++i) {
        nes_read_map[i + 0x60] = nes_sram + (i << 8);
        nes_write_map[i + 0x60] = nes_sram + (i << 8);
    }

    /* Set up the PPU */
    if(!nes_chr_rom_size)
        is_rom = 0;

    nes_ppu_set_patterntbl(0, nes_chr_rom, is_rom);
    nes_ppu_set_patterntbl(1, nes_chr_rom + 0x1000, is_rom);

    if(nes_cur_hdr.flags[0] & 0x08)
        nes_ppu_set_tblmirrors(0, 1, 2, 3);
    else if(nes_cur_hdr.flags[0] & 0x01)
        nes_ppu_set_tblmirrors(0, 1, 0, 1);
    else
        nes_ppu_set_tblmirrors(0, 0, 1, 1);
}

static void reset(void) {
}

static uint8 read_byte(void *cpu __UNUSED__, uint16 addr) {
    switch(addr >> 12) {
        case 2:
        case 3:
            return nes_ppu_readreg(addr & 0x0007);

        case 4:
            if(addr < 0x4020)
                return nes_mem_readreg(addr);
            break;
    }

    return nes_read_map[addr >> 8][(uint8)addr];
}

static void write_byte(void *cpu __UNUSED__, uint16 addr, uint8 val) {
    /* Is the write bound anywhere interesting? */
    switch(addr >> 12) {
        case 2:
        case 3:
            nes_ppu_writereg(addr & 0x0007, val);
            break;

        case 4:
            if(addr < 0x4020)
                nes_mem_writereg(addr, val);
            break;
    }

    nes_write_map[addr >> 8][(uint8)addr] = val;
}

static int write_cxt(FILE *fp __UNUSED__) {
    /* Nothing to do, no state to write. */
    return 0;
}

static int read_cxt(const uint8 *buf __UNUSED__) {
    /* We shouldn't ever get called, but just in case, return error. */
    return -1;
}

static uint32 cxt_len(void) {
    return 0;
}

/* Mapper definition. */
nes_mapper_t nes_mapper_0 = {
    0,
    &init,
    &reset,
    &read_byte,
    &write_byte,
    &write_cxt,
    &read_cxt,
    &cxt_len
};
