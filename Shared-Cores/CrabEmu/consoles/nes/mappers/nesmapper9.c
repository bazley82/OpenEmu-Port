/*
    This file is part of CrabEmu.

    Copyright (C) 2013 Lawrence Sebald

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

/* iNES Mapper 9: MMC2/PxROM */
static uint8 prg_bank = 0;
static uint8 chr_bank[4] = { 0, 0, 0, 0 };
static int latches[2] = { 0, 0 }, mirror = 0;
static int prg_banks, chr_size;

static void fill_prg_map(void) {
    int i, page = (prg_bank & 0x0F) << 5;

    for(i = 0; i < 0x20; ++i) {
        nes_read_map[i + 0x80] = nes_prg_rom + ((i + page) << 8);
    }

    nes_mem_update_map();
}

static void fill_chr_map(void) {
    int page;

    if(latches[0])
        page = (chr_bank[1] << 12) & (chr_size - 1);
    else
        page = (chr_bank[0] << 12) & (chr_size - 1);
    nes_ppu_set_patterntbl(0, nes_chr_rom + page, 1);

    if(latches[1])
        page = (chr_bank[3] << 12) & (chr_size - 1);
    else
        page = (chr_bank[2] << 12) & (chr_size - 1);

    nes_ppu_set_patterntbl(1, nes_chr_rom + page, 1);
}

static void ppu_cb(uint16 pn) {
    /* Set/clear the appropriate latch */
    if((pn & 0x00FF) == 0xFD)
        latches[(pn >> 8) & 1] = 0;
    else
        latches[(pn >> 8) & 1] = 1;

    fill_chr_map();
}

static void init(void) {
    int i, j;

    prg_banks = j = nes_prg_rom_size >> 8;
    chr_size = nes_chr_rom_size;
    prg_bank = 0;
    chr_bank[0] = chr_bank[1] = chr_bank[2] = chr_bank[3] = 0;
    latches[0] = latches[1] = mirror = 0;

    /* Fill in the read map */
    for(i = 0xFF; i > 0x9F; --i) {
        nes_read_map[i] = nes_prg_rom + (--j << 8);
    }

    for(i = 0; i < 0x20; ++i) {
        nes_read_map[i + 0x60] = nes_sram + (i << 8);
        nes_write_map[i + 0x60] = nes_sram + (i << 8);
    }

    fill_prg_map();

    /* Set up the PPU */
    nes_ppu_set_patterntbl(0, nes_chr_rom, 1);
    nes_ppu_set_patterntbl(1, nes_chr_rom, 1);
    nes_ppu_set_tblmirrors(0, 1, 0, 1);
    nes_ppu_set_mmc2(&ppu_cb);
}

static void reset(void) {
    /* Reset all the registers and the CPU's mapping */
    prg_bank = 0;
    chr_bank[0] = chr_bank[1] = chr_bank[2] = chr_bank[3] = 0;
    latches[0] = latches[1] = mirror = 0;
    fill_prg_map();

    /* Reset the PPU */
    nes_ppu_set_patterntbl(0, nes_chr_rom, 1);
    nes_ppu_set_patterntbl(1, nes_chr_rom, 1);
    nes_ppu_set_tblmirrors(0, 1, 0, 1);
    nes_ppu_set_mmc2(&ppu_cb);
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

        case 10:
            prg_bank = val;
            fill_prg_map();
            break;

        case 11:
        case 12:
        case 13:
        case 14:
            chr_bank[(addr >> 12) - 11] = val;
            fill_chr_map();
            break;

        case 15:
            if((mirror = val & 0x01))
                nes_ppu_set_tblmirrors(0, 0, 1, 1);
            else
                nes_ppu_set_tblmirrors(0, 1, 0, 1);
    }

    nes_write_map[addr >> 8][(uint8)addr] = val;
}

static int write_cxt(FILE *fp) {
    uint8 data[4];

    data[0] = 'M';
    data[1] = 'P';
    data[2] = 'P';
    data[3] = 'R';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(24, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    UINT16_TO_BUF(0, data);
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 0) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    data[0] = prg_bank;
    data[1] = (latches[0]) | (latches[1] << 1) | (mirror << 2);
    data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);

    fwrite(chr_bank, 1, 4, fp);

    return 0;
}

static int read_cxt(const uint8 *buf) {
    uint16 ver;
    uint32 len;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 24)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Read in the registers */
    prg_bank = buf[16];
    latches[0] = buf[17] & 0x01;
    latches[1] = (buf[17] & 0x02) >> 1;
    mirror = (buf[17] & 0x04) >> 2;

    chr_bank[0] = buf[20];
    chr_bank[1] = buf[21];
    chr_bank[2] = buf[22];
    chr_bank[3] = buf[23];

    fill_prg_map();
    fill_chr_map();

    if(mirror)
        nes_ppu_set_tblmirrors(0, 0, 1, 1);
    else
        nes_ppu_set_tblmirrors(0, 1, 0, 1);
    nes_ppu_set_mmc2(&ppu_cb);

    return 0;
}

static uint32 cxt_len(void) {
    return 24;
}

/* Mapper definition. */
nes_mapper_t nes_mapper_9 = {
    9,
    &init,
    &reset,
    &read_byte,
    &write_byte,
    &write_cxt,
    &read_cxt,
    &cxt_len
};
