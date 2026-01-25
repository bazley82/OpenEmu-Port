/*
    This file is part of CrabEmu.

    Copyright (C) 2010, 2012 Lawrence Sebald

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

#include <string.h>

#include "mapper-koreanmsx.h"
#include "smsz80.h"

/* This code is based in part on code from MEKA.  */

extern uint8 sms_paging_regs[4];
extern uint8 *sms_read_map[256];
extern uint8 *sms_write_map[256];
extern uint32 sms_cart_len;
extern uint8 *sms_dummy_areaw;
extern uint8 *sms_cart_rom;

static void kmsx_remap_page0(void) {
    int i;
    uint32 npgs = sms_cart_len >> 13;
    int reg = sms_paging_regs[0];
    uint8 *rom;

    reg %= npgs;
    rom = sms_cart_rom + (reg << 13);

    for(i = 0x00; i < 0x20; ++i) {
        sms_read_map[i + 0x40] = rom + (i << 8);
        sms_write_map[i + 0x40] = sms_dummy_areaw;
    }

    sms_z80_set_readmap(sms_read_map);
}

static void kmsx_remap_page1(void) {
    int i;
    uint32 npgs = sms_cart_len >> 13;
    int reg = sms_paging_regs[1];
    uint8 *rom;

    reg %= npgs;
    rom = sms_cart_rom + (reg << 13);

    for(i = 0x00; i < 0x20; ++i) {
        sms_read_map[i + 0x60] = rom + (i << 8);
        sms_write_map[i + 0x60] = sms_dummy_areaw;
    }

    sms_z80_set_readmap(sms_read_map);
}

static void kmsx_remap_page2(void) {
    int i;
    uint32 npgs = sms_cart_len >> 13;
    int reg = sms_paging_regs[2];
    uint8 *rom;

    reg %= npgs;
    rom = sms_cart_rom + (reg << 13);

    for(i = 0x00; i < 0x20; ++i) {
        sms_read_map[i + 0x80] = rom + (i << 8);
        sms_write_map[i + 0x80] = sms_dummy_areaw;
    }

    sms_z80_set_readmap(sms_read_map);
}

static void kmsx_remap_page3(void) {
    int i;
    uint32 npgs = sms_cart_len >> 13;
    int reg = sms_paging_regs[3];
    uint8 *rom;

    reg %= npgs;
    rom = sms_cart_rom + (reg << 13);

    for(i = 0x00; i < 0x20; ++i) {
        sms_read_map[i + 0xA0] = rom + (i << 8);
        sms_write_map[i + 0xA0] = sms_dummy_areaw;
    }

    sms_z80_set_readmap(sms_read_map);
}

void sms_mem_koreanmsx_remap(void) {
    int i;

    /* Map the unchanging stuff first... */
    for(i = 0x00; i < 0x40; ++i) {
        sms_read_map[i] = sms_cart_rom + (i << 8);
        sms_write_map[i] = sms_dummy_areaw;
    }

    kmsx_remap_page0();
    kmsx_remap_page1();
    kmsx_remap_page2();
    kmsx_remap_page3();
}

uint8 sms_mem_koreanmsx_mread(uint16 addr) {
    return sms_read_map[addr >> 8][addr & 0xFF];
}

void sms_mem_koreanmsx_mwrite(uint16 addr, uint8 data) {
    sms_write_map[addr >> 8][addr & 0xFF] = data;

    /* See if we were writing to any of the paging regs */
    switch(addr) {
        case 0x0000:
            sms_paging_regs[2] = data;
            kmsx_remap_page2();
            return;

        case 0x0001:
            sms_paging_regs[3] = data;
            kmsx_remap_page3();
            return;

        case 0x0002:
            sms_paging_regs[0] = data;
            kmsx_remap_page0();
            return;

        case 0x0003:
            sms_paging_regs[1] = data;
            kmsx_remap_page1();
            return;
    }
}

uint16 sms_mem_koreanmsx_mread16(uint16 addr) {
    int top = addr >> 8, bot = addr & 0xFF;
    uint16 data = sms_read_map[top][bot++];

    if(bot <= 0xFF)
        data |= (sms_read_map[top][bot] << 8);
    else
        data |= (sms_read_map[(uint8)(top + 1)][0] << 8);

    return data;
}

void sms_mem_koreanmsx_mwrite16(uint16 addr, uint16 data) {
    int top = addr >> 8, bot = addr & 0xFF;

    sms_write_map[top][bot++] = (uint8)data;

    if(bot <= 0xFF)
        sms_write_map[top][bot] = (uint8)(data >> 8);
    else
        sms_write_map[(uint8)(top + 1)][0] = (uint8)(data >> 8);

    /* See if we were writing to any of the paging regs */
    switch(addr) {
        case 0xFFFF:
            sms_paging_regs[2] = (uint8)(data >> 8);
            kmsx_remap_page2();
            return;

        case 0x0000:
            sms_paging_regs[2] = (uint8)data;
            sms_paging_regs[3] = (uint8)(data >> 8);
            kmsx_remap_page2();
            kmsx_remap_page3();
            return;
            
        case 0x0001:
            sms_paging_regs[3] = (uint8)data;
            sms_paging_regs[0] = (uint8)(data >> 8);
            kmsx_remap_page3();
            kmsx_remap_page0();
            return;
            
        case 0x0002:
            sms_paging_regs[0] = (uint8)data;
            sms_paging_regs[1] = (uint8)(data >> 8);
            kmsx_remap_page0();
            kmsx_remap_page1();
            return;
            
        case 0x0003:
            sms_paging_regs[1] = (uint8)data;
            kmsx_remap_page1();
            return;
    }
}

int sms_mem_koreanmsx_write_context(FILE *fp) {
    uint8 data[4];

    /* Write the Mapper Paging Registers block */
    data[0] = 'M';
    data[1] = 'P';
    data[2] = 'P';
    data[3] = 'R';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(20, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(sms_paging_regs, 1, 4, fp);

    return 0;
}

int sms_mem_koreanmsx_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 20)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Copy in the registers */
    memcpy(sms_paging_regs, buf + 16, 4);
    return 0;
}
