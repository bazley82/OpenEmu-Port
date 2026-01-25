/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2006, 2007, 2008, 2009, 2011, 2012 Lawrence Sebald

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

#include "mapper-93c46.h"
#include "93c46.h"

extern uint8 sms_paging_regs[4];
extern uint8 *sms_read_map[256];
extern uint8 *sms_write_map[256];
extern eeprom93c46_t e93c46;

typedef void (*remap_page_func)();
extern remap_page_func sms_mem_remap_page[4];

uint8 sms_mem_93c46_mread(uint16 addr) {
    if(e93c46.enabled && (addr >> 12) == 8) {
        if(addr == 0x8000) {
            return eeprom93c46_read();
        }
        else if(addr >= 0x8008 && addr < 0x8088) {
            int a = (addr - 0x8008) >> 1;

            if(addr & 0x01) {
                return e93c46.data[a] >> 8;
            }
            else {
                return e93c46.data[a] & 0xFF;
            }
        }
    }

    return sms_read_map[addr >> 8][addr & 0xFF];
}

void sms_mem_93c46_mwrite(uint16 addr, uint8 data) {
    sms_write_map[addr >> 8][addr & 0xFF] = data;

    if(addr == 0x8000 && e93c46.enabled) {
        eeprom93c46_write(data);
    }
    else if(addr >= 0x8008 && addr < 0x8088 && e93c46.enabled) {
        int a = (addr - 0x8008) >> 1;

        if(addr & 0x01) {
            e93c46.data[a] &= 0x00FF;
            e93c46.data[a] |= data << 8;
        }
        else {
            e93c46.data[a] &= 0xFF00;
            e93c46.data[a] |= (data);
        }
    }
    else if(addr == 0xFFFC) {
        eeprom93c46_ctl_write(data);
    }
    else if(addr > 0xFFFC) {
        /* This is bound for a paging register */
        if(sms_paging_regs[addr - 0xFFFC] != data) {
            sms_paging_regs[addr - 0xFFFC] = data;
            sms_mem_remap_page[addr - 0xFFFC]();
        }
    }
}

uint16 sms_mem_93c46_mread16(uint16 addr) {
    int top = addr >> 8, bot = addr & 0xFF;
    uint16 data;

    if(e93c46.enabled && (addr >> 12) == 8) {
        if(addr == 0x8000) {
            return eeprom93c46_read() | (sms_read_map[0x80][0x01] << 8);
        }
        else if(addr == 0x7FFF) {
            return (eeprom93c46_read() << 8) | sms_read_map[0x7F][0xFF];
        }
        else if(addr >= 0x8008 && addr < 0x8088) {
            int a = (addr - 0x8008) >> 1;

            if(addr & 0x01) {                
                if(a < 0x3F) {
                    return (e93c46.data[a] << 8) | (e93c46.data[a + 1] & 0xFF);
                }
                else {
                    return (e93c46.data[a] << 8) | sms_read_map[0x80][0x88];
                }
            }
            else {
                return e93c46.data[a];
            }
        }
    }

    data = sms_read_map[top][bot++];

    if(bot <= 0xFF)
        return data | (sms_read_map[top][bot] << 8);
    else
        return data | (sms_read_map[(uint8)(top + 1)][0] << 8);
}

void sms_mem_93c46_mwrite16(uint16 addr, uint16 data) {
    int top = addr >> 8, bot = addr & 0xFF;

    sms_write_map[top][bot++] = (uint8)data;

    if(bot <= 0xFF)
        sms_write_map[top][bot] = (uint8)(data >> 8);
    else
        sms_write_map[(uint8)(top + 1)][0] = (uint8)(data >> 8);

    if(e93c46.enabled) {
        if(addr == 0x8000) {
            eeprom93c46_write((uint8)data);
        }
        else if(addr == 0x7FFF) {
            eeprom93c46_write(data >> 8);
        }
        else if(addr >= 0x8008 && addr < 0x8088) {
            int a = (addr - 0x8008) >> 1;

            if(addr & 0x01) {
                e93c46.data[a] &= 0x00FF;
                e93c46.data[a] |= data << 8;

                if(a < 0x80) {
                    e93c46.data[a + 1] &= 0xFF00;
                    e93c46.data[a + 1] |= (uint8)data;
                }
            }
            else {
                e93c46.data[a] = data;
            }
        }
    }

    if(addr == 0xFFFC) {
        eeprom93c46_ctl_write(data);
    }
    else if(addr == 0xFFFB) {
        eeprom93c46_ctl_write(data >> 8);
    }

    if(addr > 0xFFFC) {
        /* This is bound for a paging register */
        sms_paging_regs[addr - 0xFFFC] = data;
        sms_mem_remap_page[addr - 0xFFFC]();
    }

    if(addr > 0xFFFB) {
        sms_paging_regs[addr - 0xFFFC + 1] = data >> 8;
        sms_mem_remap_page[addr - 0xFFFC + 1]();
    }
}

int sms_mem_93c46_write_context(FILE *fp) {
    uint8 data[4];
    int i;

    /* Write the Mapper Paging Registers block */
    data[0] = 'M';
    data[1] = 'P';
    data[2] = 'P';
    data[3] = 'R';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(28, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(sms_paging_regs, 1, 4, fp);
    fwrite(&e93c46.mode, 1, 1, fp);
    fwrite(&e93c46.lines, 1, 1, fp);
    fwrite(&e93c46.opcode, 1, 1, fp);
    data[0] = (e93c46.enabled ? 1 : 0) | (e93c46.readwrite ? 2 : 0);
    fwrite(data, 1, 1, fp);

    UINT16_TO_BUF(e93c46.data_in, data);
    fwrite(data, 1, 2, fp);

    data[0] = e93c46.bit;
    data[1] = 0;
    fwrite(data, 1, 2, fp);

    /* Write the Mapper RAM block */
    data[0] = 'M';
    data[1] = 'P';
    data[2] = 'R';
    data[3] = 'M';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(144, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    for(i = 0; i < 64; ++i) {
        UINT16_TO_BUF(e93c46.data[i], data);
        fwrite(data, 1, 2, fp);
    }

    return 0;
}

int sms_mem_93c46_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 28)
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
    e93c46.mode = buf[20];
    e93c46.lines = buf[21];
    e93c46.opcode = buf[22];
    e93c46.enabled = buf[23] & 1;
    e93c46.readwrite = (buf[23] & 2) ? 1 : 0;
    BUF_TO_UINT16(buf + 24, e93c46.data_in);
    e93c46.bit = buf[26];

    return 0;
}

int sms_mem_93c46_read_mem(const uint8 *buf) {
    uint32 len;
    uint16 ver;
    int i;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 144)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Read in the data */
    for(i = 0; i < 64; ++i) {
        BUF_TO_UINT16(buf + 16 + (i << 1), e93c46.data[i]);
    }

    return 0;
}
