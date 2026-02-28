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

/* iNES Mapper 1: MMC1/SxROM

   There are quite a few cases that aren't handled in this code properly as of
   yet. */

static int shift_pos = 0;
static uint8 shift_reg = 0;
static uint8 bank_regs[4];
static int chr_is_rom;
static int prg_banks;
static int chr_size;

static void fill_prg_map(void) {
    int i, page;

    /* Are we in 16 or 32KB mode? */
    if(bank_regs[0] & 0x08) {
        page = (bank_regs[3] & 0x0F) << 6;

        /* Which page should be swapped? */
        if(bank_regs[0] & 0x04) {
            for(i = 0; i < 0x40; ++i) {
                nes_read_map[i + 0x80] = nes_prg_rom +
                    (((i + page) << 8) & (nes_prg_rom_size - 1));
                nes_read_map[i + 0xC0] = nes_prg_rom +
                    (((i + 0x3C0) << 8) & (nes_prg_rom_size - 1));
            }
        }
        else {
            for(i = 0; i < 0x40; ++i) {
                nes_read_map[i + 0x80] = nes_prg_rom +
                    ((i << 8) & (nes_prg_rom_size - 1));
                nes_read_map[i + 0xC0] = nes_prg_rom +
                    (((i + page) << 8) & (nes_prg_rom_size - 1));
            }
        }
    }
    else {
        page = (bank_regs[3] & 0x0E) << 6;

        for(i = 0; i < 0x80; ++i) {
            nes_read_map[i + 0x80] = nes_prg_rom +
                (((i + page) << 8) & (nes_prg_rom_size - 1));
        }
    }

    nes_mem_update_map();
}

static void select_mirroring(void) {
    switch(bank_regs[0] & 0x03) {
        case 0:
            nes_ppu_set_tblmirrors(0, 0, 0, 0);
            break;

        case 1:
            nes_ppu_set_tblmirrors(1, 1, 1, 1);
            break;

        case 2:
            nes_ppu_set_tblmirrors(0, 1, 0, 1);
            break;

        case 3:
            nes_ppu_set_tblmirrors(0, 0, 1, 1);
            break;
    }
}

static void fill_chr_map(void) {
    int page;

    /* Which bank size do we have? */
    if(bank_regs[0] & 0x10) {
        page = (bank_regs[1] << 12) & (chr_size - 1);
        nes_ppu_set_patterntbl(0, nes_chr_rom + page, chr_is_rom);
        page = (bank_regs[2] << 12) & (chr_size - 1);
        nes_ppu_set_patterntbl(1, nes_chr_rom + page, chr_is_rom);
    }
    else {
        page = ((bank_regs[1] & 0x1E) << 12) & (chr_size - 1);
        nes_ppu_set_patterntbl(0, nes_chr_rom + page, chr_is_rom);
        nes_ppu_set_patterntbl(1, nes_chr_rom + page + 0x1000, chr_is_rom);
    }
}

static void init(void) {
    int i;

    prg_banks = nes_prg_rom_size >> 8;
    chr_size = nes_chr_rom_size;

    bank_regs[0] = 0x0C;
    bank_regs[1] = bank_regs[2] = bank_regs[3] = 0;

    for(i = 0; i < 0x20; ++i) {
        nes_read_map[i + 0x60] = nes_sram + (i << 8);
        nes_write_map[i + 0x60] = nes_sram + (i << 8);
    }

    if(!nes_chr_rom_size) {
        chr_size = 8192;
        chr_is_rom = 0;
    }

    fill_prg_map();
    select_mirroring();
    fill_chr_map();
}

static void reset(void) {
    bank_regs[0] = 0x0C;
    bank_regs[1] = bank_regs[2] = bank_regs[3] = 0;

    fill_prg_map();
    select_mirroring();
    fill_chr_map();
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

        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            if(val & 0x80) {
                shift_pos = 0;
                shift_reg = 0;
                bank_regs[0] |= 0x0C;
            }
            else {
                shift_reg |= ((val & 0x01) << shift_pos);
                ++shift_pos;

                if(shift_pos == 5) {
                    if(addr < 0xA000) {
                        bank_regs[0] = shift_reg;
                        fill_prg_map();
                        select_mirroring();
                        fill_chr_map();
                    }
                    else if(addr < 0xC000) {
                        bank_regs[1] = shift_reg;
                        fill_chr_map();
                    }
                    else if(addr < 0xE000) {
                        bank_regs[2] = shift_reg;
                        fill_chr_map();
                    }
                    else {
                        bank_regs[3] = shift_reg;
                        fill_prg_map();
                    }

                    shift_pos = 0;
                    shift_reg = 0;
                }
            }
            break;
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

    fwrite(bank_regs, 1, 4, fp);
    data[0] = shift_reg;
    data[1] = (uint8)shift_pos;
    data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);

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
    bank_regs[0] = buf[16];
    bank_regs[1] = buf[17];
    bank_regs[2] = buf[18];
    bank_regs[3] = buf[19];
    shift_reg = buf[20];
    shift_pos = (int)buf[21];

    fill_prg_map();
    select_mirroring();
    fill_chr_map();

    return 0;
}

static uint32 cxt_len(void) {
    return 24;
}

/* Mapper definition. */
nes_mapper_t nes_mapper_1 = {
    1,
    &init,
    &reset,
    &read_byte,
    &write_byte,
    &write_cxt,
    &read_cxt,
    &cxt_len
};
