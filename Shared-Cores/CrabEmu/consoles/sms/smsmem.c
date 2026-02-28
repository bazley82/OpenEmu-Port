/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014,
                  2015, 2016 Lawrence Sebald

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

#include "sms.h"
#include "smsmem.h"
#include "smsmem-gg.h"
#include "smsvdp.h"
#include "sn76489.h"
#include "ym2413.h"
#include "93c46.h"
#include "smsz80.h"
#include "mappers.h"
#include "terebi.h"
#include "cheats.h"
#include "mapper-93c46.h"
#include "mapper-korean.h"
#include "mapper-codemasters.h"
#include "mapper-sega.h"
#include "mapper-sg1000.h"
#include "mapper-koreanmsx.h"
#include "mapper-4PAA.h"
#include "mapper-none.h"
#include "mapper-janggun.h"
#include "sdscterminal.h"

#ifndef NO_ZLIB
#include "unzip.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#ifdef _arch_dreamcast
#include <inttypes.h>
#include <kos/fs.h>
#include <dc/vmufs.h>
#include <dc/vmu_pkg.h>
#include <dc/maple.h>
#include <zlib/zlib.h>
#include <bzlib/bzlib.h>
#include "icon.h"
#else
#ifndef NO_ZLIB
#include <zlib.h>
#endif

#ifndef NO_BZ2
#include <bzlib.h>
#endif
#endif

static uint8 ram[8 * 1024];

static uint8 *sms_bios_rom = NULL;
static uint32 sms_bios_len;
static int cartram_enabled = 0;

static uint32 mapper;

extern uint16 sms_pad;
extern int sms_region;
extern sn76489_t psg;
extern eeprom93c46_t e93c46;
extern int sms_ym2413_enabled;
extern YM2413 *sms_fm;

uint8 sms_paging_regs[4];
uint8 *sms_rom_page0;
uint8 *sms_rom_page1;
uint8 *sms_rom_page2;
uint8 *sms_cart_rom = NULL;
uint32 sms_cart_len;
uint8 *gg_bios_rom = NULL;
uint32 gg_bios_len;
uint8 sms_memctl = 0;
uint8 sms_ioctl = 0;
uint16 sms_ioctl_input_mask = 0xFFFF;
uint16 sms_ioctl_output_mask = 0x0000;
uint16 sms_ioctl_output_bits = 0x0000;
uint8 sms_gg_regs[7];
uint8 *sms_read_map[256];
uint8 *sms_write_map[256];
uint8 sms_dummy_arear[256];
uint8 sms_dummy_areaw[256];
uint8 sms_cart_ram[0x8000];
int sms_bios_active = 0;
int sms_control_type[2] = { SMS_PADTYPE_CONTROL_PAD, SMS_PADTYPE_CONTROL_PAD };
static uint8 sms_fm_detect = 0;
static uint8 sms_ym2413_regs[0x41] = { 0 };
static int sms_ym2413_in_use = 0;
static uint32 rom_crc, rom_adler;
static int gfx_board_nibble[2] = { 0, 0 };
uint32 sms_gfxbd_data[2] = { 0x0000000F, 0x0000000F };

typedef void (*remap_page_func)();
remap_page_func sms_mem_remap_page[4];

static int (*sms_map_write_cxt)(FILE *fp) = sms_mem_nomap_write_context;
static int (*sms_map_read_cxt)(const uint8 *buf) = sms_mem_nomap_read_context;
static int (*sms_map_read_mem)(const uint8 *buf) = sms_mem_nomap_read_context;

static void remap_page0_unmapped() {
    int i;

    for(i = 0; i < 0x40; ++i) {
        sms_read_map[i] = sms_dummy_arear;
        sms_write_map[i] = sms_dummy_areaw;
    }
}

static void remap_page1_unmapped() {
    int i;

    for(i = 0x40; i < 0x80; ++i) {
        sms_read_map[i] = sms_dummy_arear;
        sms_write_map[i] = sms_dummy_areaw;
    }
}

static void remap_page2_unmapped() {
    int i;

    for(i = 0x80; i < 0xC0; ++i) {
        sms_read_map[i] = sms_dummy_arear;
        sms_write_map[i] = sms_dummy_areaw;
    }
}

static void remap_page0() {
    int i;

    if(mapper == SMS_MAPPER_NONE || mapper == SMS_MAPPER_TW_MSX_TYPE_B) {
        sms_rom_page0 = sms_cart_rom;
    }
    else {
        if(sms_paging_regs[1] && sms_cart_len > 0x4000) {
            sms_rom_page0 = &sms_cart_rom[0x4000 * (sms_paging_regs[1] %
                                                    (sms_cart_len / 0x4000))];
        }
        else {
            sms_rom_page0 = sms_cart_rom;
        }
    }

    if(mapper != SMS_MAPPER_SEGA) {
        i = 0x00;
    }
    else {
        i = 0x04;
        sms_read_map[0] = sms_cart_rom;
        sms_read_map[1] = sms_cart_rom + 0x100;
        sms_read_map[2] = sms_cart_rom + 0x200;
        sms_read_map[3] = sms_cart_rom + 0x300;
        sms_write_map[0] = sms_dummy_areaw;
        sms_write_map[1] = sms_dummy_areaw;
        sms_write_map[2] = sms_dummy_areaw;
        sms_write_map[3] = sms_dummy_areaw;
    }

    for(; i < 0x40; ++i) {
        sms_read_map[i] = sms_rom_page0 + (i << 8);
        sms_write_map[i] = sms_dummy_areaw;
    }

    sms_z80_set_readmap(sms_read_map);
}

static void remap_page0_sms_bios() {
    int i;

    if(sms_paging_regs[1] && sms_bios_len > 0x4000) {
        sms_rom_page0 = &sms_bios_rom[0x4000 * (sms_paging_regs[1] %
                                                (sms_bios_len / 0x4000))];
    }
    else {
        sms_rom_page0 = sms_bios_rom;
    }

    sms_read_map[0] = sms_bios_rom;
    sms_read_map[1] = sms_bios_rom + 0x100;
    sms_read_map[2] = sms_bios_rom + 0x200;
    sms_read_map[3] = sms_bios_rom + 0x300;
    sms_write_map[0] = sms_dummy_areaw;
    sms_write_map[1] = sms_dummy_areaw;
    sms_write_map[2] = sms_dummy_areaw;
    sms_write_map[3] = sms_dummy_areaw;

    if(sms_bios_len >= 0x4000) {
        for(i = 0x04; i < 0x40; ++i) {
            sms_read_map[i] = sms_rom_page0 + (i << 8);
            sms_write_map[i] = sms_dummy_areaw;
        }
    }
    else {
        for(i = 0x04; i < 0x20; ++i) {
            sms_read_map[i] = sms_read_map[i + 0x20] = sms_rom_page0 + (i << 8);
            sms_write_map[i] = sms_write_map[i + 0x20] = sms_dummy_areaw;
        }
    }

    sms_z80_set_readmap(sms_read_map);
}

static void remap_page1() {
    int i;

    if(mapper == SMS_MAPPER_NONE || mapper == SMS_MAPPER_TW_MSX_TYPE_B ||
       mapper == SMS_MAPPER_TW_MSX_TYPE_A) {
        if(sms_cart_len > 0x4000) {
            sms_rom_page1 = &sms_cart_rom[0x4000];
        }
        else {
            sms_rom_page1 = sms_cart_rom;
        }

        for(i = 0x40; i < 0x80; ++i) {
            sms_read_map[i] = sms_rom_page1 + ((i & 0x3F) << 8);
            sms_write_map[i] = sms_dummy_areaw;
        }
    }
    else {
        if(sms_paging_regs[2] && sms_cart_len > 0x4000) {
            sms_rom_page1 = &sms_cart_rom[0x4000 * (sms_paging_regs[2] %
                                                    (sms_cart_len / 0x4000))];
        }
        else {
            sms_rom_page1 = sms_cart_rom;
        }

        for(i = 0x40; i < 0x80; ++i) {
            sms_read_map[i] = sms_rom_page1 + ((i & 0x3F) << 8);
            sms_write_map[i] = sms_dummy_areaw;
        }
    }

    sms_z80_set_readmap(sms_read_map);
}

static void remap_page1_sms_bios() {
    int i;

    if(sms_paging_regs[2] && sms_bios_len > 0x4000) {
        sms_rom_page1 = &sms_bios_rom[0x4000 * (sms_paging_regs[2] %
                                                (sms_bios_len / 0x4000))];
    }
    else {
        sms_rom_page1 = sms_bios_rom;
    }

    if(sms_bios_len >= 0x4000) {
        for(i = 0x40; i < 0x80; ++i) {
            sms_read_map[i] = sms_rom_page1 + ((i & 0x3F) << 8);
            sms_write_map[i] = sms_dummy_areaw;
        }
    }
    else {
        for(i = 0x40; i < 0x60; ++i) {
            sms_read_map[i] = sms_read_map[i + 0x20] =
                sms_rom_page1 + ((i & 0x3F) << 8);
            sms_write_map[i] = sms_write_map[i + 0x20] = sms_dummy_areaw;
        }
    }

    sms_z80_set_readmap(sms_read_map);
}

static void remap_page2() {
    int i, isram = 0;

    if(mapper == SMS_MAPPER_NONE || mapper == SMS_MAPPER_TW_MSX_TYPE_B ||
       mapper == SMS_MAPPER_TW_MSX_TYPE_A) {
        if(sms_cart_len > 0x8000) {
            sms_rom_page2 = &sms_cart_rom[0x8000];
        }
        else {
            sms_rom_page2 = sms_cart_rom;
        }

        for(i = 0x80; i < 0xC0; ++i) {
            sms_read_map[i] = sms_rom_page2 + ((i & 0x3F) << 8);
            sms_write_map[i] = sms_dummy_areaw;
        }
    }
    else {
        if(sms_paging_regs[0] & 0x08 && mapper == SMS_MAPPER_SEGA) {
            /* Cartridge RAM enable */
            if(sms_paging_regs[0] & 0x04) {
                sms_rom_page2 = &sms_cart_ram[0x4000];
            }
            else {
                sms_rom_page2 = &sms_cart_ram[0];
            }

            isram = 1;
            cartram_enabled = 1;
        }
        else if(sms_paging_regs[3] && sms_cart_len > 0x4000) {
            sms_rom_page2 = &sms_cart_rom[0x4000 * (sms_paging_regs[3] %
                                                    (sms_cart_len / 0x4000))];
        }
        else {
            sms_rom_page2 = sms_cart_rom;
        }

        if(isram) {
            for(i = 0x80; i < 0xC0; ++i) {
                sms_read_map[i] = sms_rom_page2 + ((i & 0x3F) << 8);
                sms_write_map[i] = sms_rom_page2 + ((i & 0x3F) << 8);
            }
        }
        else {
            for(i = 0x80; i < 0xC0; ++i) {
                sms_read_map[i] = sms_rom_page2 + ((i & 0x3F) << 8);
                sms_write_map[i] = sms_dummy_areaw;
            }
        }
    }

    sms_z80_set_readmap(sms_read_map);
}

static void remap_page2_sms_bios() {
    int i;

    /* Assume no SRAM for BIOS. */
    if(sms_paging_regs[3] && sms_bios_len > 0x4000) {
        sms_rom_page2 = &sms_bios_rom[0x4000 * (sms_paging_regs[3] %
                                                (sms_bios_len / 0x4000))];
    }
    else {
        sms_rom_page2 = sms_bios_rom;
    }

    if(sms_bios_len >= 0x4000) {
        for(i = 0x80; i < 0xC0; ++i) {
            sms_read_map[i] = sms_rom_page2 + ((i & 0x3F) << 8);
            sms_write_map[i] = sms_dummy_areaw;
        }
    }
    else {
        for(i = 0x80; i < 0xA0; ++i) {
            sms_read_map[i] = sms_read_map[i + 0x20] =
                sms_rom_page2 + ((i & 0x3F) << 8);
            sms_write_map[i] = sms_write_map[i + 0x20] = sms_dummy_areaw;
        }
    }

    sms_z80_set_readmap(sms_read_map);
}

static void reorganize_pages() {
    if(!sms_bios_active && !(sms_memctl & SMS_MEMCTL_CART)) {
        switch(mapper) {
            case SMS_MAPPER_JANGGUN:
                sms_mem_janggun_remap();
                break;

            case SMS_MAPPER_KOREAN:
                sms_mem_korean_remap();
                break;

            case SMS_MAPPER_KOREAN_MSX:
                sms_mem_koreanmsx_remap();
                break;

            default:
                sms_mem_remap_page[1]();
                sms_mem_remap_page[2]();
                sms_mem_remap_page[3]();
        }
    }
    else {
        sms_mem_remap_page[1]();
        sms_mem_remap_page[2]();
        sms_mem_remap_page[3]();
    }
}

/* This function is based on code from MEKA, the duty of this function is to
   try to autodetect what mapper type this cartridge should be using. */
static void setup_mapper() {
    int codemasters, sega, korean, korean_msx;
    uint8 *ptr;

    if(sms_cart_len <= 0x8000) {
        /* The cartridge contains less than 32KB of data, so there probably
           isn't even a mapper in it, but just to be safe, in case it has
           SRAM for some reason, we'll just assume the Sega Mapper. */
        mapper = SMS_MAPPER_SEGA;
        return;
    }

    sega = 0;
    codemasters = 0;
    korean = 0;
    korean_msx = 0;
    ptr = sms_cart_rom;

    while(ptr < sms_cart_rom + 0x8000) {
        if((*ptr++) == 0x32) {
            uint16 value = (*ptr) | ((*(ptr + 1)) << 8);

            ptr += 2;

            if(value == 0xFFFF)
                ++sega;
            else if(value == 0x8000 || value == 0x4000)
                ++codemasters;
            else if(value == 0xA000)
                ++korean;
            else if(value == 0x0002 || value == 0x0003 || value == 0x0004)
                ++korean_msx;
        }
    }

    if(korean_msx > sega + 2)
        mapper = SMS_MAPPER_KOREAN_MSX;
    else if(codemasters > sega + 2)
        mapper = SMS_MAPPER_CODEMASTERS;
    else if(korean > sega + 2)
        mapper = SMS_MAPPER_KOREAN;
    else
        mapper = SMS_MAPPER_SEGA;
}

void sms_mem_handle_memctl(uint8 data) {
    if(sms_memctl == data)
        return;

    if(sms_cons._base.console_type != CONSOLE_SMS &&
       sms_cons._base.console_type != CONSOLE_GG)
        return;

    if(!(data & SMS_MEMCTL_BIOS) &&
       sms_cons._base.console_type == CONSOLE_SMS &&
       sms_bios_rom != NULL) {
        sms_mem_remap_page[0] = &remap_page2_sms_bios;
        sms_mem_remap_page[1] = &remap_page0_sms_bios;
        sms_mem_remap_page[2] = &remap_page1_sms_bios;
        sms_mem_remap_page[3] = &remap_page2_sms_bios;
        sms_z80_set_mread(&sms_mem_sega_mread);
        sms_z80_set_mread16(&sms_mem_sega_mread16);
        sms_z80_set_mwrite(&sms_mem_sega_mwrite);
        sms_z80_set_mwrite16(&sms_mem_sega_mwrite16);
        sms_bios_active = 1;
    }
    else if((!(data & SMS_MEMCTL_CART) || sms_bios_rom == NULL) &&
            sms_cart_rom) {
        sms_bios_active = 0;

        switch(mapper) {
            case SMS_MAPPER_SEGA:
                sms_z80_set_mread(&sms_mem_sega_mread);
                sms_z80_set_mread16(&sms_mem_sega_mread16);
                sms_z80_set_mwrite(&sms_mem_sega_mwrite);
                sms_z80_set_mwrite16(&sms_mem_sega_mwrite16);
                sms_map_write_cxt = &sms_mem_sega_write_context;
                sms_map_read_cxt = &sms_mem_sega_read_context;
                sms_map_read_mem = &sms_mem_sega_read_mem;
                break;

            case SMS_MAPPER_CODEMASTERS:
                sms_z80_set_mread(&sms_mem_codemasters_mread);
                sms_z80_set_mwrite(&sms_mem_codemasters_mwrite);
                sms_z80_set_mread16(&sms_mem_codemasters_mread16);
                sms_z80_set_mwrite16(&sms_mem_codemasters_mwrite16);
                sms_mem_remap_page[0] = &sms_mem_remap_page2_codemasters;
                sms_mem_remap_page[1] = &remap_page0;
                sms_mem_remap_page[2] = &sms_mem_remap_page1_codemasters;
                sms_mem_remap_page[3] = &sms_mem_remap_page2_codemasters;
                sms_map_write_cxt = &sms_mem_codemasters_write_context;
                sms_map_read_cxt = &sms_mem_codemasters_read_context;
                sms_map_read_mem = &sms_mem_codemasters_read_mem;
                break;

            case SMS_MAPPER_KOREAN:
                sms_z80_set_mread(&sms_mem_korean_mread);
                sms_z80_set_mwrite(&sms_mem_korean_mwrite);
                sms_z80_set_mread16(&sms_mem_korean_mread16);
                sms_z80_set_mwrite16(&sms_mem_korean_mwrite16);
                sms_map_write_cxt = &sms_mem_korean_write_context;
                sms_map_read_cxt = &sms_mem_korean_read_context;
                break;

            case SMS_MAPPER_93C46:
                sms_z80_set_mread(&sms_mem_93c46_mread);
                sms_z80_set_mwrite(&sms_mem_93c46_mwrite);
                sms_z80_set_mread16(&sms_mem_93c46_mread16);
                sms_z80_set_mwrite16(&sms_mem_93c46_mwrite16);
                sms_map_write_cxt = &sms_mem_93c46_write_context;
                sms_map_read_cxt = &sms_mem_93c46_read_context;
                sms_map_read_mem = &sms_mem_93c46_read_mem;
                break;

            case SMS_MAPPER_KOREAN_MSX:
                sms_z80_set_mread(&sms_mem_koreanmsx_mread);
                sms_z80_set_mwrite(&sms_mem_koreanmsx_mwrite);
                sms_z80_set_mread16(&sms_mem_koreanmsx_mread16);
                sms_z80_set_mwrite16(&sms_mem_koreanmsx_mwrite16);
                sms_map_write_cxt = &sms_mem_koreanmsx_write_context;
                sms_map_read_cxt = &sms_mem_koreanmsx_read_context;
                break;

            case SMS_MAPPER_4PAK_ALL_ACTION:
                sms_z80_set_mread(&sms_mem_4paa_mread);
                sms_z80_set_mwrite(&sms_mem_4paa_mwrite);
                sms_z80_set_mread16(&sms_mem_4paa_mread16);
                sms_z80_set_mwrite16(&sms_mem_4paa_mwrite16);
                sms_map_write_cxt = &sms_mem_4paa_write_context;
                sms_map_read_cxt = &sms_mem_4paa_read_context;
                break;

            case SMS_MAPPER_NONE:
                sms_z80_set_mread(&sms_mem_nomap_mread);
                sms_z80_set_mwrite(&sms_mem_nomap_mwrite);
                sms_z80_set_mread16(&sms_mem_nomap_mread16);
                sms_z80_set_mwrite16(&sms_mem_nomap_mwrite16);
                sms_map_write_cxt = &sms_mem_nomap_write_context;
                sms_map_read_cxt = &sms_mem_nomap_read_context;
                break;

            case SMS_MAPPER_JANGGUN:
                sms_z80_set_mread(&sms_mem_janggun_mread);
                sms_z80_set_mwrite(&sms_mem_janggun_mwrite);
                sms_z80_set_mread16(&sms_mem_janggun_mread16);
                sms_z80_set_mwrite16(&sms_mem_janggun_mwrite16);
                sms_map_write_cxt = &sms_mem_janggun_write_context;
                sms_map_read_cxt = &sms_mem_janggun_read_context;
                break;
        }

        if(mapper != SMS_MAPPER_CODEMASTERS) {
            sms_mem_remap_page[0] = &remap_page2;
            sms_mem_remap_page[1] = &remap_page0;
            sms_mem_remap_page[2] = &remap_page1;
            sms_mem_remap_page[3] = &remap_page2;
        }
    }
    else {
        sms_bios_active = 0;
        sms_mem_remap_page[0] = &remap_page2_unmapped;
        sms_mem_remap_page[1] = &remap_page0_unmapped;
        sms_mem_remap_page[2] = &remap_page1_unmapped;
        sms_mem_remap_page[3] = &remap_page2_unmapped;
    }

    reorganize_pages();

    sms_memctl = data;
}

void sms_mem_handle_ioctl(uint8 data) {
    int old, new;
    uint8 tmp;

    /* Make sure we're emulating an export SMS, the Japanese SMS (and earlier
       hardware) did not have the I/O Control Register functionality. */
    if((sms_cons._base.console_type == CONSOLE_SMS &&
        (sms_region & SMS_REGION_DOMESTIC)) ||
       sms_cons._base.console_type == CONSOLE_SG1000)
        return;

    tmp = sms_ioctl;
    sms_ioctl = data;
    old = ((sms_pad & sms_ioctl_input_mask) |
           (sms_ioctl_output_bits & sms_ioctl_output_mask)) ^ SMS_TH_MASK;

    /* Reset the input/output masks. */
    sms_ioctl_input_mask = 0x37DF;

    sms_ioctl_input_mask |= ((data & SMS_IOCTL_TR_A_DIRECTION) << 5) |
                            ((data & SMS_IOCTL_TH_A_DIRECTION) << 13) |
                            ((data & SMS_IOCTL_TR_B_DIRECTION) << 9) |
                            ((data & SMS_IOCTL_TH_B_DIRECTION) << 12);
   sms_ioctl_output_mask = sms_ioctl_input_mask ^ 0xFFFF;

    /* And grab the output levels of the bits specified. */
   sms_ioctl_output_bits = ((data & SMS_IOCTL_TR_A_LEVEL) << 1) |
                           ((data & SMS_IOCTL_TH_A_LEVEL) << 9) |
                           ((data & SMS_IOCTL_TR_B_LEVEL) << 5) |
                           ((data & SMS_IOCTL_TH_B_LEVEL) << 8);

   new = ((sms_pad & sms_ioctl_input_mask) |
          (sms_ioctl_output_bits & sms_ioctl_output_mask)) & SMS_TH_MASK;

   if(old & new)
       sms_vdp_hcnt_latch(sms_cycles_elapsed() % SMS_CYCLES_PER_LINE);

    /* Handle any graphics boards hooked up. Information from the SMS Power
       forums provided by Maxim and Bock here:
       http://www.smspower.org/forums/viewtopic.php?p=81920#81920 */
    if(sms_control_type[0] == SMS_PADTYPE_GFX_BOARD) {
        if((tmp ^ data) & SMS_IOCTL_TH_A_LEVEL)
            gfx_board_nibble[0] = (gfx_board_nibble[0] + 1) & 7;
        if(data & SMS_IOCTL_TR_A_LEVEL)
            gfx_board_nibble[0] = 0;
    }

    if(sms_control_type[1] == SMS_PADTYPE_GFX_BOARD) {
        if((tmp ^ data) & SMS_IOCTL_TH_B_LEVEL)
            gfx_board_nibble[1] = (gfx_board_nibble[1] + 1) & 7;
        if(data & SMS_IOCTL_TR_B_LEVEL)
            gfx_board_nibble[1] = 0;
    }
}

static uint8 sms_read_controls(uint16 port) {
    uint16 output = sms_ioctl_output_bits & sms_ioctl_output_mask;
    uint16 input = sms_pad;

    /* The information here came from the forum thread linked to above... */
    if(sms_control_type[0] == SMS_PADTYPE_GFX_BOARD) {
        if(sms_ioctl & SMS_IOCTL_TR_A_LEVEL) {
            input &= 0xFFE0;
        }
        else {
            input = (input & 0xFFF0) |
                ((sms_gfxbd_data[0] >> (gfx_board_nibble[0] << 2)) & 0x0F);

            if(gfx_board_nibble[0] == 0)
                input &= ~SMS_PAD1_TL;
        }
    }

    /* This is totally a guess until someone actually tests this out, but it
       seems logical, at least. */
    if(sms_control_type[1] == SMS_PADTYPE_GFX_BOARD) {
        if(sms_ioctl & SMS_IOCTL_TR_B_LEVEL) {
            input &= 0xF83F;
        }
        else {
            input = (input & 0xFC3F) |
                (((sms_gfxbd_data[1] >> (gfx_board_nibble[1] << 2)) &
                  0x0F) <<6);

            if(gfx_board_nibble[1] == 0)
                input &= ~SMS_PAD2_TL;
        }
    }

    input &= sms_ioctl_input_mask;

    if(port & 0x01) {
        /* I/O port B/misc register */
        return ((input | output) >> 8) & 0xFF;
    }
    else {
        /* I/O port A/B register */
        return (input | output) & 0xFF;
    }
}

void sms_port_write(uint16 port, uint8 data) {
    port &= 0xFF;

    if(port < 0x40) {
        if(port & 0x01) {
            /* I/O Control register */
            sms_mem_handle_ioctl(data);
        }
        else {
            /* Memory Control register */
            sms_mem_handle_memctl(data);
        }
    }
    else if(port < 0x80) {
        /* SN76489 PSG */
        sn76489_write(&psg, data);
    }
    else if(port < 0xC0) {
        if(port & 0x01) {
            /* VDP Control port */
            sms_vdp_ctl_write(data);
        }
        else {
            /* VDP Data port */
            sms_vdp_data_write(data);
        }
    }
    else {
        if(sms_ym2413_enabled) {
            if(port == 0xF0) {
                sms_ym2413_regs[0x40] = data;
                //YM2413Write(0, 0, data);
                ym2413_write(sms_fm, 0, data);
                sms_ym2413_in_use = 1;
            }
            else if(port == 0xF1) {
                sms_ym2413_regs[sms_ym2413_regs[0x40]] = data;
                //YM2413Write(0, 1, data);
                ym2413_write(sms_fm, 1, data);
                sms_ym2413_in_use = 1;
            }
            else if(port == 0xF2) {
                sms_ym2413_in_use = 1;
                sms_fm_detect = data;
            }
        }

#ifdef ENABLE_SDSC_TERMINAL
        if(sms_memctl & SMS_MEMCTL_IO) {
            if(port == 0xFC) {
                sms_sdsc_ctl_write(data);
            }
            else if(port == 0xFD) {
                sms_sdsc_data_write(data);
            }
        }
#endif
    }
}

uint8 sms_port_read(uint16 port) {
    port &= 0xFF;

    if(port < 0x40) {
        return 0xFF;
    }
    else if(port < 0x80) {
        if(port & 0x01) {
            return sms_vdp_hcnt_read();
        }
        else {
            return sms_vdp_vcnt_read();
        }
    }
    else if(port < 0xC0) {
        if(port & 0x01) {
            return sms_vdp_status_read();
        }
        else {
            return sms_vdp_data_read();
        }
    }
    else {
        /* Uguu~~ Some games apparently don't disable the I/O chip properly when
           trying to detect the FM unit... In theory this would potentially give
           some fun interference on the bus and allow for false positives, but
           whatever. */
        if(sms_ym2413_enabled && port == 0xF2) {
            sms_ym2413_in_use = 1;
            return sms_fm_detect;
        }
        else if(!(sms_memctl & SMS_MEMCTL_IO)) {
            return sms_read_controls(port);
        }
        else {
            return 0xFF;
        }
    }
}

#ifndef _arch_dreamcast

int sms_write_cartram_to_file(const char *fn) {
    FILE *fp;

    if(fn == NULL)
        return -1;

    if(mapper == SMS_MAPPER_93C46) {
        fp = fopen(fn, "wb");

        if(!fp)
            return -1;

        fwrite(e93c46.data, 64, 2, fp);
        fclose(fp);
    }
    else {
        if(!cartram_enabled)
            return 0;

        fp = fopen(fn, "wb");
        if(!fp)
            return -1;

        fwrite(sms_cart_ram, 0x8000, 1, fp);
        fclose(fp);
    }

    return 0;
}

int sms_read_cartram_from_file(const char *fn) {
    FILE * fp;

    if(fn == NULL)
        return -1;

    fp = fopen(fn, "rb");

    if(fp != NULL) {
        if(mapper == SMS_MAPPER_93C46) {
            eeprom93c46_init();
            fread(e93c46.data, 64, 2, fp);
        }
        else {
            fread(sms_cart_ram, 0x8000, 1, fp);
            cartram_enabled = 1;
        }
        fclose(fp);

        return 0;
    }

    return -1;
}

#else

int sms_write_cartram_to_file(const char *fn) {
    vmu_pkg_t pkg;
    uint8 *pkg_out, *comp;
    int pkg_size, err;
    uint32 len;
    FILE *fp;
    char savename[32];
    maple_device_t *vmu;
    int rv = 0, blocks_freed = 0;
    file_t f;

    if(!cartram_enabled && mapper != SMS_MAPPER_93C46)
        return 0;

    /* Make sure there's a VMU in port A1. */
    if(!(vmu = maple_enum_dev(0, 1))) {
        return -100;
    }

    if(!vmu->valid || !(vmu->info.functions & MAPLE_FUNC_MEMCARD)) {
        return -100;
    }

    sprintf(savename, "/vmu/a1/ce-%08" PRIX32, (uint32_t)rom_crc);

    comp = (uint8 *)malloc(0x10000);
    len = 0x10000;

    if(mapper == SMS_MAPPER_93C46) {
        err = compress2(comp, &len, (uint8 *)e93c46.data, 128, 9);
    }
    else {
        err = compress2(comp, &len, sms_cart_ram, 0x8000, 9);
    }

    if(err != Z_OK) {
        free(comp);
        return -1;
    }

    if(fn != NULL) {
        strncpy(pkg.desc_long, fn, 32);
        pkg.desc_long[31] = 0;
    }
    else {
        sprintf(pkg.desc_long, "CRC: %08" PRIX32 " Adler: %08" PRIX32,
                (uint32_t)rom_crc, (uint32_t)rom_adler);
    }

    strcpy(pkg.desc_short, "CrabEmu Save");
    strcpy(pkg.app_id, "CrabEmu");
    pkg.icon_cnt = 1;
    pkg.icon_anim_speed = 0;
    memcpy(pkg.icon_pal, icon_pal, 32);
    pkg.icon_data = icon_img;
    pkg.eyecatch_type = VMUPKG_EC_NONE;
    pkg.data_len = len;
    pkg.data = comp;

    vmu_pkg_build(&pkg, &pkg_out, &pkg_size);

    /* See if a file exists with that name, since we'll overwrite it. */
    f = fs_open(savename, O_RDONLY);
    if(f != FILEHND_INVALID) {
        blocks_freed = fs_total(f) >> 9;
        fs_close(f);
    }

    /* Make sure there's enough free space on the VMU. */
    if(vmufs_free_blocks(vmu) + blocks_freed < (pkg_size >> 9)) {
        free(pkg_out);
        free(comp);
        return pkg_size >> 9;
    }

    if(!(fp = fopen(savename, "wb"))) {
        free(pkg_out);
        free(comp);
        return -1;
    }

    if(fwrite(pkg_out, 1, pkg_size, fp) != (size_t)pkg_size)
        rv = -1;

    fclose(fp);

    free(pkg_out);
    free(comp);

    return rv;
}

int sms_read_cartram_from_file(const char *fn __UNUSED__) {
    vmu_pkg_t pkg;
    uint8 *pkg_out;
    int pkg_size;
    char prodcode[7];
    FILE *fp;
    char savename[32];
    uint32 real_size = mapper == SMS_MAPPER_93C46 ? 128 : 0x8000;

    /* Try the newer filename first... */
    sprintf(savename, "/vmu/a1/ce-%08" PRIX32, (uint32_t)rom_crc);

    if(!(fp = fopen(savename, "rb"))) {
        /* Failing that, try the product-code based filename that was used in
           CrabEmu 0.1.9 and earlier... */
        sprintf(prodcode, "%d%d%d%d%d", (sms_cart_rom[0x7FFE] & 0xF0) >> 4,
                (sms_cart_rom[0x7FFD] & 0xF0) >> 4,
                (sms_cart_rom[0x7FFD] & 0x0F),
                (sms_cart_rom[0x7FFC] & 0xF0) >> 4,
                (sms_cart_rom[0x7FFC] & 0x0F));

        sprintf(savename, "/vmu/a1/%s", prodcode);

        if(!(fp = fopen(savename, "rb")))
            return -1;
    }

    fseek(fp, 0, SEEK_SET);
    fseek(fp, 0, SEEK_END);
    pkg_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    pkg_out = (uint8 *)malloc(pkg_size);
    fread(pkg_out, pkg_size, 1, fp);
    fclose(fp);

    vmu_pkg_parse(pkg_out, &pkg);

    if(mapper == SMS_MAPPER_93C46) {
        eeprom93c46_init();
        uncompress((uint8 *)e93c46.data, &real_size, pkg.data, pkg.data_len);
    }
    else {
        uncompress(sms_cart_ram, &real_size, pkg.data, pkg.data_len);
    }

    free(pkg_out);

    cartram_enabled = 1;

    return 0;
}

#endif /* _arch_dreamcast */

int sms_mem_load_bios(const char *fn) {
    FILE *fp;
    int size;
    char *ext;
    uint8 **ptr;
    uint32 *len;

    fp = fopen(fn, "rb");

    if(!fp) {
#ifdef DEBUG
        fprintf(stderr, "sms_mem_load_bios: Could not open ROM: %s!\n", fn);
#endif
        return -1;
    }

    /* First, determine which type of BIOS ROM it is */
    ext = strrchr(fn, '.');

    if(!strcmp(ext, ".gg")) {
        /* Game Gear BIOS */
        ptr = &gg_bios_rom;
        len = &gg_bios_len;
    }
    else {
        /* Master System BIOS, hopefully */
        ptr = &sms_bios_rom;
        len = &sms_bios_len;
    }

    if(*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
        *len = 0;
    }

    /* Determine the size of the file */
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Assume there's no footers or headers. */

    *ptr = (uint8 *)malloc(size);
    fread(*ptr, size, 1, fp);
    fclose(fp);

    *len = size;

    /* Assume the mapper is the Sega mapper, if any is even needed. */

    return 0;
}

static void finalize_load(const char *fn) {
    char *name;
    int i;

    /* Determine which mapper should be used, and set up the paging
       accordingly. First check the mappers database */
    if((mapper = sms_find_mapper(sms_cart_rom, sms_cart_len, &rom_crc,
                                 &rom_adler)) == (uint32)-1) {
        mapper = SMS_MAPPER_SEGA;

        if(sms_cons._base.console_type != CONSOLE_SG1000) {
            setup_mapper();

            if(mapper == SMS_MAPPER_SEGA) {
                /* Check if this game uses the 93c46 serial eeprom, and if so
                   enable it. Most of these should be detected by the
                   sms_find_mapper function, but just in case, detect them
                   here as well. */
                const uint16 prodcodes[5] = {
                    0x3432, 0x2537, 0x2439, 0x3407, 0x2418
                };
                const uint16 popbreaker = 0x2017;
                uint16 prodcode = sms_cart_rom[0x7FFC] |
                    (sms_cart_rom[0x7FFD] << 8) |
                    ((sms_cart_rom[0x7FFE] & 0xF0) << 12);

                for(i = 0; i < 5; ++i) {
                    if(prodcode == prodcodes[i]) {
                        mapper = SMS_MAPPER_93C46;
                        break;
                    }
                }

                /* While we're at it, check if its Pop Breaker, which apparently
                   doesn't like running on a non-japanese Game Gear */
                if(prodcode == popbreaker) {
                    sms_gg_regs[0] = 0x80;
                }
            }
        }
    }

#ifdef DEBUG
    printf("Detected Mapper %d\n", mapper);
#endif

    if((sms_cons._base.console_type == CONSOLE_SG1000 ||
        sms_cons._base.console_type == CONSOLE_SC3000) &&
       mapper != SMS_MAPPER_TW_MSX_TYPE_B) {
        for(i = 0; i < 0x08; ++i) {
            sms_read_map[i + 0xC8] = ram + (i << 8);
            sms_read_map[i + 0xD0] = ram + (i << 8);
            sms_read_map[i + 0xD8] = ram + (i << 8);
            sms_read_map[i + 0xE8] = ram + (i << 8);
            sms_read_map[i + 0xF0] = ram + (i << 8);
            sms_read_map[i + 0xF8] = ram + (i << 8);

            sms_write_map[i + 0xC8] = ram + (i << 8);
            sms_write_map[i + 0xD0] = ram + (i << 8);
            sms_write_map[i + 0xD8] = ram + (i << 8);
            sms_write_map[i + 0xE8] = ram + (i << 8);
            sms_write_map[i + 0xF0] = ram + (i << 8);
            sms_write_map[i + 0xF8] = ram + (i << 8);
        }
    }

    if(mapper == SMS_MAPPER_TW_MSX_TYPE_B) {
        for(i = 0; i < 0x20; ++i) {
            sms_read_map[i + 0xC0] = sms_cart_ram + (i << 8);
            sms_read_map[i + 0xE0] = sms_cart_ram + (i << 8);
            sms_write_map[i + 0xC0] = sms_cart_ram + (i << 8);
            sms_write_map[i + 0xE0] = sms_cart_ram + (i << 8);
        }
    }

    if((sms_cons._base.console_type != CONSOLE_SMS || sms_bios_rom == NULL) &&
       (sms_cons._base.console_type != CONSOLE_GG || gg_bios_rom == NULL)) {
        switch(mapper) {
            case SMS_MAPPER_SEGA:
                if(sms_cons._base.console_type != CONSOLE_SG1000) {
                    sms_z80_set_mread(&sms_mem_sega_mread);
                    sms_z80_set_mread16(&sms_mem_sega_mread16);
                    sms_z80_set_mwrite(&sms_mem_sega_mwrite);
                    sms_z80_set_mwrite16(&sms_mem_sega_mwrite16);
                    sms_map_write_cxt = &sms_mem_sega_write_context;
                    sms_map_read_cxt = &sms_mem_sega_read_context;
                    sms_map_read_mem = &sms_mem_sega_read_mem;
                }
                else {
                    sms_z80_set_mread(&sms_mem_sg_mread);
                    sms_z80_set_mread16(&sms_mem_sg_mread16);
                    sms_z80_set_mwrite(&sms_mem_sg_mwrite);
                    sms_z80_set_mwrite16(&sms_mem_sg_mwrite16);
                    sms_map_write_cxt = &sms_mem_nomap_write_context;
                    sms_map_read_cxt = &sms_mem_nomap_read_context;
                }
                break;

            case SMS_MAPPER_CODEMASTERS:
                sms_z80_set_mread(&sms_mem_codemasters_mread);
                sms_z80_set_mwrite(&sms_mem_codemasters_mwrite);
                sms_z80_set_mread16(&sms_mem_codemasters_mread16);
                sms_z80_set_mwrite16(&sms_mem_codemasters_mwrite16);
                sms_mem_remap_page[0] = &sms_mem_remap_page2_codemasters;
                sms_mem_remap_page[1] = &remap_page0;
                sms_mem_remap_page[2] = &sms_mem_remap_page1_codemasters;
                sms_mem_remap_page[3] = &sms_mem_remap_page2_codemasters;
                sms_map_write_cxt = &sms_mem_codemasters_write_context;
                sms_map_read_cxt = &sms_mem_codemasters_read_context;
                sms_map_read_mem = &sms_mem_codemasters_read_mem;
                break;

            case SMS_MAPPER_KOREAN:
                sms_z80_set_mread(&sms_mem_korean_mread);
                sms_z80_set_mwrite(&sms_mem_korean_mwrite);
                sms_z80_set_mread16(&sms_mem_korean_mread16);
                sms_z80_set_mwrite16(&sms_mem_korean_mwrite16);
                sms_map_write_cxt = &sms_mem_korean_write_context;
                sms_map_read_cxt = &sms_mem_korean_read_context;
                break;

            case SMS_MAPPER_93C46:
                eeprom93c46_init();
                sms_z80_set_mread(&sms_mem_93c46_mread);
                sms_z80_set_mwrite(&sms_mem_93c46_mwrite);
                sms_z80_set_mread16(&sms_mem_93c46_mread16);
                sms_z80_set_mwrite16(&sms_mem_93c46_mwrite16);
                sms_map_write_cxt = &sms_mem_93c46_write_context;
                sms_map_read_cxt = &sms_mem_93c46_read_context;
                sms_map_read_mem = &sms_mem_93c46_read_mem;
                break;

            case SMS_MAPPER_CASTLE:
                sms_z80_set_mread(&sms_mem_sg_mread);
                sms_z80_set_mwrite(&sms_mem_sg_mwrite);
                sms_z80_set_mread16(&sms_mem_sg_mread16);
                sms_z80_set_mwrite16(&sms_mem_sg_mwrite16);
                sms_mem_remap_page[3] = &sms_mem_remap_page2_castle;
                sms_map_write_cxt = &sms_mem_8kb_write_context;
                sms_map_read_cxt = &sms_mem_nomap_read_context;
                sms_map_read_mem = &sms_mem_8kb_read_mem;
                break;

            case SMS_MAPPER_TEREBI_OEKAKI:
                sms_z80_set_mread(&terebi_mread);
                sms_z80_set_mwrite(&terebi_mwrite);
                sms_z80_set_mread16(&terebi_mread16);
                sms_z80_set_mwrite16(&terebi_mwrite16);
                sms_map_write_cxt = &terebi_write_context;
                sms_map_read_cxt = &terebi_read_context;
                break;

            case SMS_MAPPER_KOREAN_MSX:
                sms_z80_set_mread(&sms_mem_koreanmsx_mread);
                sms_z80_set_mwrite(&sms_mem_koreanmsx_mwrite);
                sms_z80_set_mread16(&sms_mem_koreanmsx_mread16);
                sms_z80_set_mwrite16(&sms_mem_koreanmsx_mwrite16);
                sms_map_write_cxt = &sms_mem_koreanmsx_write_context;
                sms_map_read_cxt = &sms_mem_koreanmsx_read_context;
                break;

            case SMS_MAPPER_4PAK_ALL_ACTION:
                sms_z80_set_mread(&sms_mem_4paa_mread);
                sms_z80_set_mwrite(&sms_mem_4paa_mwrite);
                sms_z80_set_mread16(&sms_mem_4paa_mread16);
                sms_z80_set_mwrite16(&sms_mem_4paa_mwrite16);
                sms_map_write_cxt = &sms_mem_4paa_write_context;
                sms_map_read_cxt = &sms_mem_4paa_read_context;
                break;

            case SMS_MAPPER_NONE:
                sms_z80_set_mread(&sms_mem_nomap_mread);
                sms_z80_set_mwrite(&sms_mem_nomap_mwrite);
                sms_z80_set_mread16(&sms_mem_nomap_mread16);
                sms_z80_set_mwrite16(&sms_mem_nomap_mwrite16);
                sms_map_write_cxt = &sms_mem_nomap_write_context;
                sms_map_read_cxt = &sms_mem_nomap_read_context;
                break;

            case SMS_MAPPER_TW_MSX_TYPE_B:
                sms_z80_set_mread(&sms_mem_nomap_mread);
                sms_z80_set_mwrite(&sms_mem_nomap_mwrite);
                sms_z80_set_mread16(&sms_mem_nomap_mread16);
                sms_z80_set_mwrite16(&sms_mem_nomap_mwrite16);
                sms_map_write_cxt = &sms_mem_8kb_write_context;
                sms_map_read_cxt = &sms_mem_nomap_read_context;
                sms_map_read_mem = &sms_mem_8kb_read_mem;
                break;

            case SMS_MAPPER_JANGGUN:
                sms_z80_set_mread(&sms_mem_janggun_mread);
                sms_z80_set_mwrite(&sms_mem_janggun_mwrite);
                sms_z80_set_mread16(&sms_mem_janggun_mread16);
                sms_z80_set_mwrite16(&sms_mem_janggun_mwrite16);
                sms_mem_janggun_init();
                sms_map_write_cxt = &sms_mem_janggun_write_context;
                sms_map_read_cxt = &sms_mem_janggun_read_context;
                break;

            case SMS_MAPPER_TW_MSX_TYPE_A:
                sms_z80_set_mread(&sms_mem_sg_mread);
                sms_z80_set_mwrite(&sms_mem_sg_mwrite);
                sms_z80_set_mread16(&sms_mem_sg_mread16);
                sms_z80_set_mwrite16(&sms_mem_sg_mwrite16);
                sms_mem_remap_page[1] = &sms_mem_remap_page0_twmsxa;
                sms_map_write_cxt = &sms_mem_8kb_write_context;
                sms_map_read_cxt = &sms_mem_nomap_read_context;
                sms_map_read_mem = &sms_mem_8kb_read_mem;
                break;
        }

        /* Give a sane default for the SP since we don't have a BIOS. */
        sms_z80_write_reg(SMS_Z80_REG_SP, 0xDFF0);
    }
    else if(sms_cons._base.console_type == CONSOLE_SMS) {
        sms_mem_remap_page[0] = &remap_page2_sms_bios;
        sms_mem_remap_page[1] = &remap_page0_sms_bios;
        sms_mem_remap_page[2] = &remap_page1_sms_bios;
        sms_mem_remap_page[3] = &remap_page2_sms_bios;
        sms_z80_set_mread(&sms_mem_sega_mread);
        sms_z80_set_mread16(&sms_mem_sega_mread16);
        sms_z80_set_mwrite(&sms_mem_sega_mwrite);
        sms_z80_set_mwrite16(&sms_mem_sega_mwrite16);

        if(mapper == SMS_MAPPER_JANGGUN) {
            sms_mem_janggun_init();
        }
    }
    else if(sms_cons._base.console_type == CONSOLE_GG) {
        sms_mem_remap_page[0] = &remap_page2;
        sms_mem_remap_page[1] = &sms_mem_remap_page0_gg_bios;
        sms_mem_remap_page[2] = &remap_page1;
        sms_mem_remap_page[3] = &remap_page2;
        sms_z80_set_mread(&sms_mem_sega_mread);
        sms_z80_set_mread16(&sms_mem_sega_mread16);
        sms_z80_set_mwrite(&sms_mem_sega_mwrite);
        sms_z80_set_mwrite16(&sms_mem_sega_mwrite16);
    }

    reorganize_pages();

    name = strrchr(fn, '/');

    if(name != NULL) {
        /* we have a filename, but the first char is /, so skip that */
        gui_set_title(name + 1);
    }
    else {
        gui_set_title("CrabEmu");
    }
}

int sms_mem_run_bios(int console) {
    /* Clear cartram, although it shouldn't be relevant... */
    memset(sms_cart_ram, 0, 0x8000);
    cartram_enabled = 0;

    sms_set_console(console);

    sms_cart_rom = NULL;
    sms_cart_len = 0;

    /* Set up the mapping functions. */
    sms_mem_remap_page[0] = &remap_page2_sms_bios;
    sms_mem_remap_page[1] = &remap_page0_sms_bios;
    sms_mem_remap_page[2] = &remap_page1_sms_bios;
    sms_mem_remap_page[3] = &remap_page2_sms_bios;
    sms_z80_set_mread(&sms_mem_sega_mread);
    sms_z80_set_mread16(&sms_mem_sega_mread16);
    sms_z80_set_mwrite(&sms_mem_sega_mwrite);
    sms_z80_set_mwrite16(&sms_mem_sega_mwrite16);

    reorganize_pages();
    gui_set_title("CrabEmu");

    return 0;
}

#ifndef NO_ZLIB
static int load_gz_rom(const char *fn) {
    int len;
    gzFile fp;
    void *tmp;

    /* Open up the file in question. */
    if(!(fp = gzopen(fn, "rb"))) {
#ifdef DEBUG
        fprintf(stderr, "sms_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_OPEN;
    }

    /* Read one chunk of the file at a time, until we have no more left. */
    sms_cart_rom = (uint8 *)malloc(0x1000);

    if(!sms_cart_rom) {
#ifdef DEBUG
        fprintf(stderr, "sms_mem_load_rom: Couldn't allocate space!\n");
#endif
        gzclose(fp);
        return ROM_LOAD_E_ERRNO;
    }

    sms_cart_len = 0;
    len = gzread(fp, sms_cart_rom, 0x1000);

    while(len != 0 && len != -1) {
        sms_cart_len += len;
        tmp = realloc(sms_cart_rom, sms_cart_len + 0x1000);

        if(!tmp) {
#ifdef DEBUG
            fprintf(stderr, "sms_mem_load_rom: Couldn't allocate space!\n");
#endif
            gzclose(fp);
            free(sms_cart_rom);
            sms_cart_rom = NULL;
            return ROM_LOAD_E_ERRNO;
        }

        sms_cart_rom = (uint8 *)tmp;
        memset(sms_cart_rom + sms_cart_len, 0xFF, 0x1000);
        len = gzread(fp, sms_cart_rom + sms_cart_len, 0x1000);
    }

    gzclose(fp);

    /* Remove any SMD headers. */
    if((sms_cart_len % 0x4000) == 512) {
        /* SMD header present, skip it */
        memmove(sms_cart_rom, sms_cart_rom + 512, sms_cart_len - 512);
        sms_cart_len -= 512;
    }

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
        fprintf(stderr, "sms_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_OPEN;
    }

    /* Read one chunk of the file at a time, until we have no more left. */
    sms_cart_rom = (uint8 *)malloc(0x1000);

    if(!sms_cart_rom) {
#ifdef DEBUG
        fprintf(stderr, "sms_mem_load_rom: Couldn't allocate space!\n");
#endif
        BZ2_bzclose(fp);
        return ROM_LOAD_E_ERRNO;
    }

    sms_cart_len = 0;
    len = BZ2_bzread(fp, sms_cart_rom, 0x1000);

    while(len != 0 && len != -1) {
        sms_cart_len += len;
        tmp = realloc(sms_cart_rom, sms_cart_len + 0x1000);

        if(!tmp) {
#ifdef DEBUG
            fprintf(stderr, "sms_mem_load_rom: Couldn't allocate space!\n");
#endif
            BZ2_bzclose(fp);
            free(sms_cart_rom);
            sms_cart_rom = NULL;
            return ROM_LOAD_E_ERRNO;
        }

        sms_cart_rom = (uint8 *)tmp;
        memset(sms_cart_rom + sms_cart_len, 0xFF, 0x1000);
        len = BZ2_bzread(fp, sms_cart_rom + sms_cart_len, 0x1000);
    }

    BZ2_bzclose(fp);

    /* Remove any SMD headers. */
    if((sms_cart_len % 0x4000) == 512) {
        /* SMD header present, skip it */
        memmove(sms_cart_rom, sms_cart_rom + 512, sms_cart_len - 512);
        sms_cart_len -= 512;
    }

    finalize_load(fn);

    return ROM_LOAD_SUCCESS;
}
#endif

#ifndef NO_ZLIB
static int load_zip_rom(const char *fn) {
    int console = 0;
    unzFile fp;
    unz_file_info inf;
    char fn2[4096];
    char *ext;

    /* Open up the file in question. */
    fp = unzOpen(fn);

    if(!fp) {
#ifdef DEBUG
        fprintf(stderr, "sms_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_OPEN;
    }

    if(unzGoToFirstFile(fp) != UNZ_OK) {
#ifdef DEBUG
        fprintf(stderr, "sms_mem_load_rom: Couldn't find file in zip!\n");
#endif
        unzClose(fp);
        return ROM_LOAD_E_CORRUPT;
    }

    /* Keep looking at files until we find a rom. */
    while(console == 0) {
        if(unzGetCurrentFileInfo(fp, &inf, fn2, 4096, NULL, 0, NULL,
                                 0) != UNZ_OK) {
#ifdef DEBUG
            fprintf(stderr, "sms_mem_load_rom: Error parsing Zip file!\n");
#endif
            unzClose(fp);
            return ROM_LOAD_E_CORRUPT;
        }

        /* Check if this file looks like a rom. */
        ext = strrchr(fn2, '.');

        if(ext) {
            if(!strcmp(ext, ".gg")) {
                /* GameGear rom */
                console = CONSOLE_GG;
            }
            else if(!strcmp(ext, ".sc")) {
                /* SC-3000 rom */
                console = CONSOLE_SC3000;
            }
            else if(!strcmp(ext, ".sg")) {
                /* SG-1000 rom */
                console = CONSOLE_SG1000;
            }
            else if(!strcmp(ext, ".sms") || !strcmp(ext, ".bin")) {
                /* Master system rom, hopefully */
                console = CONSOLE_SMS;
            }
        }

        /* If we haven't found a rom yet, move to the next file. */
        if(console == 0) {
            if(unzGoToNextFile(fp) != UNZ_OK) {
#ifdef DEBUG
                fprintf(stderr, "sms_mem_load_rom: End of zip file!\n");
#endif
                unzClose(fp);
                return ROM_LOAD_E_NO_ROM;
            }
        }
    }

    /* At this point, we must have what we believe to be a rom found. */
    sms_set_console(console);

    /* Read a chunk of the file, until we have no more left. */
    sms_cart_rom = (uint8 *)malloc(inf.uncompressed_size);

    if(!sms_cart_rom) {
#ifdef DEBUG
        fprintf(stderr, "sms_mem_load_rom: Couldn't allocate space!\n");
#endif
        unzClose(fp);
        return ROM_LOAD_E_ERRNO;
    }

    sms_cart_len = inf.uncompressed_size;

    /* Attempt to open the file we think is a rom. */
    if(unzOpenCurrentFile(fp) != UNZ_OK) {
#ifdef DEBUG
        fprintf(stderr, "sms_mem_load_rom: Couldn't open rom from zip!\n");
#endif
        unzClose(fp);
        return ROM_LOAD_E_CORRUPT;
    }

    /* Attempt to read the file. */
    if(unzReadCurrentFile(fp, sms_cart_rom, sms_cart_len) < 0) {
#ifdef DEBUG
        fprintf(stderr, "sms_mem_load_rom: Couldn't read rom from zip!\n");
#endif
        unzCloseCurrentFile(fp);
        unzClose(fp);
        return ROM_LOAD_E_CORRUPT;
    }

    /* We're done with the file, close everything. */
    unzCloseCurrentFile(fp);
    unzClose(fp);

    /* Remove any SMD headers. */
    if((sms_cart_len % 0x4000) == 512) {
        /* SMD header present, skip it */
        memmove(sms_cart_rom, sms_cart_rom + 512, sms_cart_len - 512);
        sms_cart_len -= 512;
    }

    finalize_load(fn);

    return ROM_LOAD_SUCCESS;
}
#endif

int sms_mem_load_rom(const char *fn, int console) {
    FILE *fp;
    int size;
    char *ext;

    memset(sms_cart_ram, 0, 0x8000);
    cartram_enabled = 0;

    sms_set_console(console);

    /* If its a compressed format, go onto their handler... */
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
        fprintf(stderr, "sms_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_ERRNO;
    }

    /* Determine the size of the file */
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Determine if header/footer info is present */
    if((size % 0x4000) == 512) {
        /* SMD header present, skip it */
        fseek(fp, 512, SEEK_SET);
        size -= 512;
    }
    else if((size % 0x4000) == 64) {
        /* Footer present, truncate it */
        size -= 64;
    }

    sms_cart_rom = (uint8 *)malloc(size);
    fread(sms_cart_rom, size, 1, fp);
    fclose(fp);

    sms_cart_len = size;

    finalize_load(fn);

    return ROM_LOAD_SUCCESS;
}

int sms_ym2413_write_context(FILE *fp) {
    uint8 data[4];

    /* Export SMS consoles don't have this at all. */
    if(!(sms_region & SMS_REGION_DOMESTIC))
        return 0;

    /* Also, this isn't possible on Game Gear */
    if(sms_cons._base.console_type == CONSOLE_GG)
        return 0;

    /* Finally, if its not being used, don't bother */
    if(!sms_ym2413_in_use)
        return 0;

    data[0] = '2';
    data[1] = '4';
    data[2] = '1';
    data[3] = '3';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(84, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(sms_ym2413_regs, 1, 65, fp);
    data[0] = data[1] = data[2] = 0;
    fwrite(data, 1, 3, fp);

    return 0;
}

int sms_ym2413_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;
    int i;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 84)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Copy in the registers */
    memcpy(sms_ym2413_regs, buf + 16, 65);

    /* This is based on how SMS Plus handles things... */
    ym2413_write(sms_fm, 0, 0x0E);
    ym2413_write(sms_fm, 1, sms_ym2413_regs[0x0E]);

    for(i = 0x00; i <= 0x07; ++i) {
        ym2413_write(sms_fm, 0, i);
        ym2413_write(sms_fm, 1, sms_ym2413_regs[i]);
    }

    for(i = 0x10; i <= 0x18; ++i) {
        ym2413_write(sms_fm, 0, i);
        ym2413_write(sms_fm, 1, sms_ym2413_regs[i]);
    }

    for(i = 0x20; i <= 0x28; ++i) {
        ym2413_write(sms_fm, 0, i);
        ym2413_write(sms_fm, 1, sms_ym2413_regs[i]);
    }

    for(i = 0x30; i <= 0x38; ++i) {
        ym2413_write(sms_fm, 0, i);
        ym2413_write(sms_fm, 1, sms_ym2413_regs[i]);
    }

    ym2413_write(sms_fm, 0, sms_ym2413_regs[64]);

    return 0;
}

static int sms_mapper_write_context(FILE *fp) {
    uint8 data[4];
    uint32 len;

    data[0] = 'M';
    data[1] = 'A';
    data[2] = 'P';
    data[3] = 'R';
    fwrite(data, 1, 4, fp);             /* Block ID */

    switch(mapper) {
        case SMS_MAPPER_NONE:
            len = 20;
            break;

        case SMS_MAPPER_SEGA:
            /* Punt, for now. */
            len = 20 + 20 + 0x8010;
            break;

        case SMS_MAPPER_CODEMASTERS:
            /* We'll punt here too... */
            len = 20 + 20 + 8208;
            break;

        case SMS_MAPPER_93C46:
            len = 20 + 28 + 144;
            break;

        case SMS_MAPPER_JANGGUN:
            len = 20 + 28;
            break;

        /* 8KiB RAM mappers */
        case SMS_MAPPER_CASTLE:
        case SMS_MAPPER_TW_MSX_TYPE_A:
        case SMS_MAPPER_TW_MSX_TYPE_B:
            len = 20 + 8208;
            break;

        /* All of the blocks that just have a set of 4 (or less) paging regs */
        case SMS_MAPPER_KOREAN:
        case SMS_MAPPER_TEREBI_OEKAKI:
        case SMS_MAPPER_KOREAN_MSX:
        case SMS_MAPPER_4PAK_ALL_ACTION:
            len = 20 + 20;
            break;

        default:
            return -1;
    }

    UINT32_TO_BUF(len, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    UINT32_TO_BUF(4, data);
    fwrite(data, 1, 4, fp);             /* Child pointer */

    UINT32_TO_BUF(mapper, data);
    fwrite(data, 1, 4, fp);

    /* Handle mapper data/memory */
    return sms_map_write_cxt(fp);
}

int sms_mem_write_context(FILE *fp) {
    uint8 data[4];
    uint32 len;
    int memlen;

    /* Write the RAM block */
    data[0] = 'D';
    data[1] = 'R';
    data[2] = 'A';
    data[3] = 'M';
    fwrite(data, 1, 4, fp);             /* Block ID */

    switch(sms_cons._base.console_type) {
        case CONSOLE_SMS:
        case CONSOLE_GG:
            memlen = 0x2000;
            len = 0x2000 + 16;
            break;

        case CONSOLE_SG1000:
        case CONSOLE_SC3000:
            memlen = 0x0800;
            len = 0x0800 + 16;
            break;

        default:
            return -1;
    }

    UINT32_TO_BUF(len, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(ram, 1, memlen, fp);

    /* Write the SMS Registers block */
    data[0] = 'S';
    data[1] = 'M';
    data[2] = 'S';
    data[3] = 'R';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(20, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    data[0] = sms_ioctl;
    data[1] = sms_memctl;
    data[2] = sms_fm_detect;
    data[3] = 0;
    fwrite(data, 1, 4, fp);

    /* Write the GG Registers block, if appropriate */
    if(sms_cons._base.console_type == CONSOLE_GG) {
        data[0] = 'G';
        data[1] = 'G';
        data[2] = 'R';
        data[3] = 'G';
        fwrite(data, 1, 4, fp);             /* Block ID */

        UINT32_TO_BUF(24, data);
        fwrite(data, 1, 4, fp);             /* Length */

        UINT16_TO_BUF(1, data);
        fwrite(data, 1, 2, fp);             /* Version */
        fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

        data[0] = data[1] = data[2] = data[3] = 0;
        fwrite(data, 1, 4, fp);             /* Child pointer */

        fwrite(sms_gg_regs, 1, 7, fp);
        data[0] = 0;
        fwrite(data, 1, 1, fp);
    }

    return sms_mapper_write_context(fp);
}

int sms_game_write_context(FILE *fp) {
    uint8 data[4];

    data[0] = 'G';
    data[1] = 'A';
    data[2] = 'M';
    data[3] = 'E';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(24, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    UINT16_TO_BUF(0, data);
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 0) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    UINT32_TO_BUF(rom_crc, data);
    fwrite(data, 1, 4, fp);

    UINT32_TO_BUF(rom_adler, data);
    fwrite(data, 1, 4, fp);

    return 0;
}

int sms_game_read_context(const uint8 *buf) {
    uint32 len, crc;
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

    /* Check if the game matches... */
    BUF_TO_UINT32(buf + 16, crc);
    if(crc != rom_crc)
        return -2;

    BUF_TO_UINT32(buf + 20, crc);
    if(crc != rom_adler)
        return -2;

    return 0;
}

int sms_mem_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    switch(sms_cons._base.console_type) {
        case CONSOLE_SMS:
        case CONSOLE_GG:
            if(len != 0x2010)
                return -1;
            break;

        case CONSOLE_SG1000:
        case CONSOLE_SC3000:
            if(len != 0x0810)
                return -1;
            break;

        default:
            return -1;
    }

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Copy in the memory */
    memcpy(ram, buf + 16, len - 16);
    return 0;
}

int sms_regs_read_context(const uint8 *buf) {
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
    sms_mem_handle_ioctl(buf[16]);
    sms_mem_handle_memctl(buf[17]);
    sms_fm_detect = buf[18];

    return 0;
}

int sms_ggregs_read_context(const uint8 *buf) {
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
    memcpy(sms_gg_regs, buf + 16, 7);
    return 0;
}

int sms_mapper_read_context(const uint8 *buf) {
    uint32 len, rd_mapper, child, clen;
    uint16 ver;
    const uint8 *ptr;
    int rv;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len < 20)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    BUF_TO_UINT32(buf + 12, child);
    if(child != 0 && child != 4)
        return -1;

    /* Check that the mapper is what we think it is... */
    BUF_TO_UINT32(buf + 16, rd_mapper);
    if(mapper != rd_mapper)
        return -1;

    /* Handle child nodes */
    ptr = buf + 16 + child;

    while(ptr < buf + len) {
        rv = 0;

        /* Check for a malformed child node */
        if(ptr + 16 > buf + len)
            return -2;

        child = FOURCC_TO_UINT32(ptr[0], ptr[1], ptr[2], ptr[3]);
        BUF_TO_UINT32(ptr + 4, clen);

        /* Make sure the length is sane */
        if(ptr + clen > buf + len)
            return -2;

        switch(child) {
            case FOURCC_TO_UINT32('M', 'P', 'P', 'R'):
                rv = sms_map_read_cxt(ptr);
                break;

            case FOURCC_TO_UINT32('M', 'P', 'R', 'M'):
                rv = sms_map_read_mem(ptr);
                break;

            default:
                /* See if its marked as essential... */
                BUF_TO_UINT16(ptr + 10, ver);
                if(ver & 1)
                    rv = -1;
        }

        if(rv)
            return rv;

        ptr += clen;
    }

    reorganize_pages();

    return 0;
}

void sms_mem_read_context_v1(FILE *fp) {
    fread(sms_paging_regs, 4, 1, fp);
    fread(sms_gg_regs, 7, 1, fp);
    fread(ram, 0x2000, 1, fp);
    fread(sms_cart_ram, 0x8000, 1, fp);
    reorganize_pages();
}

int sms_mem_init(void) {
    int i;

    sms_cart_rom = NULL;
    sms_rom_page0 = NULL;
    sms_rom_page1 = NULL;
    sms_rom_page2 = NULL;

    sms_mem_remap_page[0] = &remap_page2;
    sms_mem_remap_page[1] = &remap_page0;
    sms_mem_remap_page[2] = &remap_page1;
    sms_mem_remap_page[3] = &remap_page2;

    sms_paging_regs[0] = 0;
    sms_paging_regs[1] = 0;
    sms_paging_regs[2] = 1;
    sms_paging_regs[3] = 0;

    sms_gg_regs[0] = 0xC0;
    sms_gg_regs[1] = 0x7F;
    sms_gg_regs[2] = 0xFF;
    sms_gg_regs[3] = 0x00;
    sms_gg_regs[4] = 0xFF;
    sms_gg_regs[5] = 0x00;
    sms_gg_regs[6] = 0xFF;

    for(i = 0x00; i < 0xC0; ++i) {
        sms_read_map[i] = sms_dummy_arear;
        sms_write_map[i] = sms_dummy_areaw;
    }

    for(i = 0xC0; i < 0xE0; ++i) {
        sms_read_map[i] = ram + ((i - 0xC0) << 8);
        sms_write_map[i] = ram + ((i - 0xC0) << 8);
    }

    for(i = 0xE0; i < 0x100; ++i) {
        sms_read_map[i] = ram + ((i - 0xE0) << 8);
        sms_write_map[i] = ram + ((i - 0xE0) << 8);
    }

    for(i = 0; i < 256; ++i) {
        sms_dummy_arear[i] = 0xFF;
    }

    sms_ioctl_input_mask = 0xFFFF;
    sms_ioctl_output_mask = 0x0000;
    sms_ioctl_output_bits = 0x0000;

    sms_memctl = (SMS_MEMCTL_IO | SMS_MEMCTL_CART | SMS_MEMCTL_RAM) ^ 0xFC;
    sms_ioctl = SMS_IOCTL_TR_A_DIRECTION | SMS_IOCTL_TH_A_DIRECTION |
                SMS_IOCTL_TR_B_DIRECTION | SMS_IOCTL_TH_B_DIRECTION;

    gfx_board_nibble[0] = gfx_board_nibble[1] = 0;
    sms_gfxbd_data[0] = sms_gfxbd_data[1] = 0x0000000F;

    memset(ram, 0xF0, 8 * 1024);

    /* Set the memctl value at address 0xC000 of the SMS' memory. */
    ram[0] = sms_memctl;
    sms_bios_active = 0;

    return 0;
}

int sms_mem_shutdown(void) {
    sms_mem_janggun_shutdown();

    if(sms_cart_rom != NULL)
        free(sms_cart_rom);

    if(sms_bios_rom != NULL)
        free(sms_bios_rom);

    if(gg_bios_rom != NULL)
        free(gg_bios_rom);

    sms_bios_rom = NULL;
    gg_bios_rom = NULL;
    sms_cart_rom = NULL;

    return 0;
}

void sms_mem_reset(void) {
    sms_paging_regs[0] = 0;
    sms_paging_regs[1] = 0;
    sms_paging_regs[2] = 1;
    sms_paging_regs[3] = 0;

    sms_gg_regs[0] |= 0x80;
    sms_gg_regs[1] = 0x7F;
    sms_gg_regs[2] = 0xFF;
    sms_gg_regs[3] = 0x00;
    sms_gg_regs[4] = 0xFF;
    sms_gg_regs[5] = 0x00;
    sms_gg_regs[6] = 0xFF;

    if(sms_cons._base.console_type == CONSOLE_SMS && sms_bios_rom != NULL) {
        sms_mem_remap_page[0] = &remap_page2_sms_bios;
        sms_mem_remap_page[1] = &remap_page0_sms_bios;
        sms_mem_remap_page[2] = &remap_page1_sms_bios;
        sms_mem_remap_page[3] = &remap_page2_sms_bios;
        sms_memctl = (SMS_MEMCTL_IO | SMS_MEMCTL_BIOS | SMS_MEMCTL_RAM) ^ 0xFC;
        sms_bios_active = 1;
    }
    else if(sms_cons._base.console_type == CONSOLE_GG && gg_bios_rom != NULL) {
        sms_mem_remap_page[0] = &remap_page2;
        sms_mem_remap_page[1] = &sms_mem_remap_page0_gg_bios;
        sms_mem_remap_page[2] = &remap_page1;
        sms_mem_remap_page[3] = &remap_page2;
        sms_memctl = (SMS_MEMCTL_IO | SMS_MEMCTL_BIOS | SMS_MEMCTL_RAM) ^ 0xFC;
        sms_bios_active = 1;
    }
    else {
        sms_memctl = (SMS_MEMCTL_IO | SMS_MEMCTL_CART | SMS_MEMCTL_RAM) ^ 0xFC;
        sms_bios_active = 0;
    }

    sms_ioctl_input_mask = 0xFFFF;
    sms_ioctl_output_mask = 0x0000;
    sms_ioctl_output_bits = 0x0000;
    sms_ioctl = SMS_IOCTL_TR_A_DIRECTION | SMS_IOCTL_TH_A_DIRECTION |
                SMS_IOCTL_TR_B_DIRECTION | SMS_IOCTL_TH_B_DIRECTION;

    gfx_board_nibble[0] = gfx_board_nibble[1] = 0;
    sms_gfxbd_data[0] = sms_gfxbd_data[1] = 0x0000000F;

    memset(ram, 0xF0, 8 * 1024);

    /* Set the memctl value at address 0xC000 in the SMS' memory. */
    ram[0] = sms_memctl;

    reorganize_pages();
    sms_mem_janggun_reset();
}

void sms_get_checksums(uint32 *crc, uint32 *adler) {
    *crc = rom_crc;
    *adler = rom_adler;
}
