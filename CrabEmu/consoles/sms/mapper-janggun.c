/*
    This file is part of CrabEmu.

    Copyright (C) 2012 Lawrence Sebald

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

#include "mapper-janggun.h"
#include "smsz80.h"

/* From smsmem.c */
extern uint8 *sms_read_map[256];
extern uint8 *sms_write_map[256];
extern uint8 *sms_cart_rom;
extern uint32 sms_cart_len;
extern uint8 sms_dummy_areaw[256];

/* Mapper data */
static uint8 *jg_cart;
static int jg_init = 0;
static int jg_npgs = 0;

/* Values that need to be saved in a save state. */
static uint8 jg_regs[6];

static void jg_remap_page2(void) {
    int i;
    uint8 *rom = sms_cart_rom;
    int reg = jg_regs[0];

    if(jg_regs[4])
        rom = jg_cart;

    for(i = 0; i < 0x20; ++i) {
        sms_read_map[i + 0x40] = rom + (reg << 13) + (i << 8);
        sms_write_map[i + 0x40] = sms_dummy_areaw;
    }

    sms_z80_set_readmap(sms_read_map);
}

static void jg_remap_page3(void) {
    int i;
    uint8 *rom = sms_cart_rom;
    int reg = jg_regs[1];

    if(jg_regs[4])
        rom = jg_cart;

    for(i = 0; i < 0x20; ++i) {
        sms_read_map[i + 0x60] = rom + (reg << 13) + (i << 8);
        sms_write_map[i + 0x60] = sms_dummy_areaw;
    }

    sms_z80_set_readmap(sms_read_map);
}

static void jg_remap_page4(void) {
    int i;
    uint8 *rom = sms_cart_rom;
    int reg = jg_regs[2];

    if(jg_regs[5])
        rom = jg_cart;

    for(i = 0; i < 0x20; ++i) {
        sms_read_map[i + 0x80] = rom + (reg << 13) + (i << 8);
        sms_write_map[i + 0x80] = sms_dummy_areaw;
    }

    sms_z80_set_readmap(sms_read_map);
}

static void jg_remap_page5(void) {
    int i;
    uint8 *rom = sms_cart_rom;
    int reg = jg_regs[3];

    if(jg_regs[5])
        rom = jg_cart;

    for(i = 0; i < 0x20; ++i) {
        sms_read_map[i + 0xA0] = rom + (reg << 13) + (i << 8);
        sms_write_map[i + 0xA0] = sms_dummy_areaw;
    }

    sms_z80_set_readmap(sms_read_map);
}

void sms_mem_janggun_remap(void) {
    int i;

    for(i = 0; i < 0x40; ++i) {
        sms_read_map[i] = &sms_cart_rom[i << 8];
        sms_write_map[i] = sms_dummy_areaw;
    }

    /* Set up the read map */
    jg_remap_page2();
    jg_remap_page3();
    jg_remap_page4();
    jg_remap_page5();
}

uint8 sms_mem_janggun_mread(uint16 addr) {
    return sms_read_map[addr >> 8][addr & 0xFF];
}

uint16 sms_mem_janggun_mread16(uint16 addr) {
    int top = addr >> 8, bot = addr & 0xFF;
    uint16 data = sms_read_map[top][bot++];

    if(bot <= 0xFF)
        data |= (sms_read_map[top][bot] << 8);
    else
        data |= (sms_read_map[(uint8)(top + 1)][0] << 8);

    return data;
}

void sms_mem_janggun_mwrite(uint16 addr, uint8 data) {
    uint8 d1, d2;

    sms_write_map[addr >> 8][addr & 0xFF] = data;
    d1 = data % jg_npgs;

    if(addr == 0x4000 && jg_regs[0] != d1) {
        jg_regs[0] = d1;
        jg_remap_page2();
    }
    else if(addr == 0x6000 && jg_regs[1] != d1) {
        jg_regs[1] = d1;
        jg_remap_page3();
    }
    else if(addr == 0x8000 && jg_regs[2] != d1) {
        jg_regs[2] = d1;
        jg_remap_page4();
    }
    else if(addr == 0xA000 && jg_regs[3] != d1) {
        jg_regs[3] = d1;
        jg_remap_page5();
    }
    else if(addr == 0xFFFE) {
        d1 = (d1 << 1) % jg_npgs;
        d2 = (d1 + 1) % jg_npgs;

        if(data & 0x40) {
            jg_regs[0] = d1;
            jg_regs[1] = d2;
            jg_regs[4] = 1;
        }
        else {
            jg_regs[0] = d1;
            jg_regs[1] = d2;
            jg_regs[4] = 0;
        }

        jg_remap_page2();
        jg_remap_page3();
    }
    else if(addr == 0xFFFF) {
        d1 = (d1 << 1) % jg_npgs;
        d2 = (d1 + 1) % jg_npgs;

        if(data & 0x40) {
            jg_regs[2] = d1;
            jg_regs[3] = d2;
            jg_regs[5] = 1;
        }
        else {
            jg_regs[2] = d1;
            jg_regs[3] = d2;
            jg_regs[5] = 0;
        }

        jg_remap_page4();
        jg_remap_page5();
    }
}

void sms_mem_janggun_mwrite16(uint16 addr, uint16 data) {
    /* Lazy! Lazy! Lazy! Not that its really that much less efficient this way,
       in the long run... */
    sms_mem_janggun_mwrite(addr, (uint8)data);
    sms_mem_janggun_mwrite(addr + 1, (uint8)(data >> 8));
}

int sms_mem_janggun_init(void) {
    uint32 i;
    uint8 b;
    uint8 jg_lut[256];

    if(jg_init)
        return 0;

    /* Generate a LUT for reversing bytes. Algorithm for determining the entry
       from:
    http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits */
    for(i = 0; i < 256; ++i) {
        b = (uint8)i;
        b = (uint8)(((b * 0x0802LU & 0x22110LU) |
                     (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16);
        jg_lut[i] = b;
    }

    /* Reset the paging registers to a sane state. */
    jg_regs[0] = 2;                 /* 0x4000 */
    jg_regs[1] = 3;                 /* 0x6000 */
    jg_regs[2] = 4;                 /* 0x8000 */
    jg_regs[3] = 5;                 /* 0xA000 */
    jg_regs[4] = 0;                 /* Bit 6 of 0xFFFE */
    jg_regs[5] = 0;                 /* Bit 6 of 0xFFFF */

    /* Generate a swapped version of the ROM */
    if(!(jg_cart = (uint8 *)malloc(sms_cart_len))) {
        return -1;
    }

    for(i = 0; i < sms_cart_len; ++i) {
        jg_cart[i] = jg_lut[sms_cart_rom[i]];
    }

    jg_npgs = sms_cart_len >> 13;
    sms_mem_janggun_remap();
    jg_init = 1;

    return 0;
}

void sms_mem_janggun_shutdown(void) {
    if(jg_init) {
        free(jg_cart);
        jg_cart = NULL;
    }

    jg_npgs = 0;
    jg_init = 0;
}

void sms_mem_janggun_reset(void) {
    if(!jg_init)
        return;

    /* Reset the paging registers to a sane state. */
    jg_regs[0] = 2;
    jg_regs[1] = 3;
    jg_regs[2] = 4;
    jg_regs[3] = 5;
    jg_regs[4] = 0;
    jg_regs[5] = 0;

    sms_mem_janggun_remap();
}

int sms_mem_janggun_write_context(FILE *fp) {
    uint8 data[4];

    /* Write the Mapper Paging Registers block */
    data[0] = 'M';
    data[1] = 'P';
    data[2] = 'P';
    data[3] = 'R';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(24, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(jg_regs, 1, 6, fp);

    data[0] = data[1] = 0;
    fwrite(data, 1, 2, fp);

    return 0;
}

int sms_mem_janggun_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

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

    /* Copy in the registers */
    memcpy(jg_regs, buf + 16, 6);
    return 0;
}
