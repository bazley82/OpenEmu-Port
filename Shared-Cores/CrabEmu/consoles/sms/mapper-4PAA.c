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
#include <string.h>

#include "mapper-4PAA.h"

extern uint8 sms_paging_regs[4];
extern uint8 *sms_read_map[256];
extern uint8 *sms_write_map[256];

typedef void (*remap_page_func)();
extern remap_page_func sms_mem_remap_page[4];

uint8 sms_mem_4paa_mread(uint16 addr) {
    return sms_read_map[addr >> 8][addr & 0xFF];
}

void sms_mem_4paa_mwrite(uint16 addr, uint8 data) {
    uint8 tmp;

    sms_write_map[addr >> 8][addr & 0xFF] = data;

    if(addr == 0x3FFE && sms_paging_regs[1] != data) {
        sms_paging_regs[1] = data;
        sms_mem_remap_page[1]();
    }
    else if(addr == 0x7FFF && sms_paging_regs[2] != data) {
        sms_paging_regs[2] = data;
        sms_mem_remap_page[2]();
    }
    else if(addr == 0xBFFF) {
        tmp = (sms_paging_regs[1] & 0x30) + data;

        if(sms_paging_regs[3] != tmp) {
            sms_paging_regs[3] = tmp;
            sms_mem_remap_page[3]();
        }
    }
}

uint16 sms_mem_4paa_mread16(uint16 addr) {
    int top = addr >> 8, bot = addr & 0xFF;
    uint16 data = sms_read_map[top][bot++];

    if(bot <= 0xFF)
        data |= (sms_read_map[top][bot] << 8);
    else
        data |= (sms_read_map[(uint8)(top + 1)][0] << 8);

    return data;
}

void sms_mem_4paa_mwrite16(uint16 addr, uint16 data) {
    int top = addr >> 8, bot = addr & 0xFF;
    uint8 tdata = (uint8)data, bdata = (uint8)(data >> 8), tmp;

    sms_write_map[top][bot++] = tdata;

    if(bot <= 0xFF)
        sms_write_map[top][bot] = bdata;
    else
        sms_write_map[(uint8)(top + 1)][0] = bdata;

    if(addr == 0x3FFE && sms_paging_regs[1] != tdata) {
        sms_paging_regs[1] = tdata;
        sms_mem_remap_page[1]();
    }
    else if(addr == 0x7FFF && sms_paging_regs[2] != tdata) {
        sms_paging_regs[2] = tdata;
        sms_mem_remap_page[2]();
    }
    else if(addr == 0xBFFF) {
        tmp = (sms_paging_regs[1] & 0x30) + tdata;

        if(sms_paging_regs[3] != tmp) {
            sms_paging_regs[3] = tmp;
            sms_mem_remap_page[3]();
        }
    }
    else if(addr == 0x3FFD && sms_paging_regs[1] != bdata) {
        sms_paging_regs[1] = bdata;
        sms_mem_remap_page[1]();
    }
    else if(addr == 0x7FFE && sms_paging_regs[2] != bdata) {
        sms_paging_regs[2] = bdata;
        sms_mem_remap_page[2]();
    }
    else if(addr == 0xBFFE) {
        tmp = (sms_paging_regs[1] & 0x30) + bdata;

        if(sms_paging_regs[3] != tmp) {
            sms_paging_regs[3] = tmp;
            sms_mem_remap_page[3]();
        }
    }
}

int sms_mem_4paa_write_context(FILE *fp) {
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

int sms_mem_4paa_read_context(const uint8 *buf) {
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
