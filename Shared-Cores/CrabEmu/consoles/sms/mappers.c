/*
    This file is part of CrabEmu.

    Copyright (C) 2008, 2012, 2015 Lawrence Sebald

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

#ifdef _arch_dreamcast
#include <zlib/zlib.h>
#else
#ifndef NO_ZLIB
#include <zlib.h>
#endif
#endif

#ifdef DEBUG
#include <stdio.h>
#include <inttypes.h>
#endif

#include "smsmem.h"
#include "mappers.h"
#include "rom.h"

typedef struct srom_s {
    uint32 adler32;
    uint32 crc32;
    uint32 mapper;
} special_rom_t;

static const int rom_count = 27;
static special_rom_t romlist[] = {
    { 0xBF3A0EDC, 0x092F29D6, SMS_MAPPER_CASTLE }, /* The Castle - SG-1000 */
    { 0x86429577, 0x4ED45BDA, SMS_MAPPER_93C46 }, /* Nomo's World Series Baseball */
    { 0x503B0A79, 0x578A8A38, SMS_MAPPER_93C46 }, /* World Series Baseball 95 */
    { 0x7216AA8E, 0x3D8D0DD6, SMS_MAPPER_93C46 }, /* World Series Baseball v1.0 */
    { 0xAB42DB3F, 0xBB38CFD7, SMS_MAPPER_93C46 }, /* World Series Baseball v1.1 */
    { 0x52A77FCF, 0x36EBCD6D, SMS_MAPPER_93C46 }, /* The Majors Pro Baseball */
    { 0x09455AFE, 0x2DA8E943, SMS_MAPPER_93C46 }, /* Pro Yakyuu GG League */
    { 0x7C31B39D, 0xDD4A661B, SMS_MAPPER_TEREBI_OEKAKI }, /* Terebi Oekaki - SG-1000 */
    { 0x6ADA90C2, 0xA67F2A5C, SMS_MAPPER_4PAK_ALL_ACTION }, /* 4 PAK ALL ACTION */
    { 0x19D2D995, 0x565C799F, SMS_MAPPER_NONE }, /* Xyzolog */
    { 0x550C10EF, 0x192949D5, SMS_MAPPER_JANGGUN }, /* Janggun-ui Adeul */
    { 0x19133E3E, 0x9FA727A0, SMS_MAPPER_CODEMASTERS }, /* Street Hero [Proto, Earlier] */
    { 0x5D4E6C70, 0xCE5648C3, SMS_MAPPER_TW_MSX_TYPE_A }, /* Bomberman Special [DahJee] */
    { 0xBEF93720, 0x223397A1, SMS_MAPPER_TW_MSX_TYPE_A }, /* King's Valley */
    { 0x1D8C9D60, 0x281D2888, SMS_MAPPER_TW_MSX_TYPE_A }, /* Knightmare [Jumbo] */
    { 0x8B78AAF6, 0x2E7166D5, SMS_MAPPER_TW_MSX_TYPE_A }, /* Legend of Kage, The [DahJee] */
    { 0x9A772AB9, 0x306D5F78, SMS_MAPPER_TW_MSX_TYPE_A }, /* Rally-X [DahJee] */
    { 0x581F7810, 0x29E047CC, SMS_MAPPER_TW_MSX_TYPE_A }, /* Road Fighter [Jumbo] */
    { 0x77E82ACD, 0x5CBD1163, SMS_MAPPER_TW_MSX_TYPE_A }, /* Tank Battalion [DahJee] */
    { 0xFFE89CAE, 0xC550B4F0, SMS_MAPPER_TW_MSX_TYPE_A }, /* TwinBee [Jumbo] */
    { 0xD66F1F96, 0x69FC1494, SMS_MAPPER_TW_MSX_TYPE_B }, /* Bomberman Special */
    { 0x7237ADB2, 0x2E366CCF, SMS_MAPPER_TW_MSX_TYPE_B }, /* Castle, The [MSX] */
    { 0x8E3A84D6, 0xFFC4EE3F, SMS_MAPPER_TW_MSX_TYPE_B }, /* Magical Kid Wiz */
    { 0x9CABE756, 0xAAAC12CF, SMS_MAPPER_TW_MSX_TYPE_B }, /* Rally-X */
    { 0x7C5AC4A0, 0xD2EDD329, SMS_MAPPER_TW_MSX_TYPE_B }, /* Road Fighter */
    { 0x6E11F0D2, 0x72542786, SMS_MAPPER_NONE }, /* Monaco GP */
    { 0x1282E24F, 0x704F6A61, SMS_MAPPER_NONE }, /* Lander 2 (homebrew) */
};

uint32 sms_find_mapper(const uint8 *rom, uint32 len, uint32 *rcrc,
                       uint32 *radler) {
    uint32 adler, crc;
    int i;

    adler = rom_adler32(rom, len);
    crc = rom_crc32(rom, len);

#ifdef DEBUG
    printf("Checksums: Adler-32: 0x%08" PRIX32 " CRC32: 0x%08" PRIX32 "\n",
           (uint32_t)adler, (uint32_t)crc);
#endif

    if(rcrc)
        *rcrc = crc;

    if(radler)
        *radler = adler;

    for(i = 0; i < rom_count; ++i) {
#ifndef NO_ZLIB
        if(romlist[i].adler32 == adler && romlist[i].crc32 == crc) {
            return romlist[i].mapper;
        }
#else
        if(romlist[i].crc32 == crc) {
            return romlist[i].mapper;
        }
#endif
    }

    return (uint32)-1;
}
