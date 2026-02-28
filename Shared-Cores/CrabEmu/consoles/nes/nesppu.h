/*
    This file is part of CrabEmu.

    Copyright (C) 2012, 2013, 2014 Lawrence Sebald

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

#ifndef NESPPU_H
#define NESPPU_H

#include "CrabEmu.h"
#include <stdio.h>

CLINKAGE

#include "nespalette.h"

/* Pattern structure */
typedef struct nespat_s {
    uint8 texture[4][64];
    uint8 dirty;
} nes_ppu_pattern_t;

extern void nes_ppu_update_cache(int pat);
extern const nes_ppu_pattern_t *nes_ppu_fetch_pattern(int pat);
extern void nes_ppu_fetch_bg_pal(uint8 pal[16]);
extern void nes_ppu_fetch_spr_pal(uint8 pal[16]);

extern void *nes_ppu_framebuffer(void);
extern void nes_ppu_framesize(uint32_t *x, uint32_t *y);
extern void nes_ppu_activeframe(uint32_t *x, uint32_t *y, uint32_t *w,
                                uint32_t *h);

extern void nes_ppu_writereg(int reg, uint8 value);
extern uint8 nes_ppu_readreg(int reg);

extern void nes_ppu_write_oamdma(uint8 val);

extern void nes_ppu_vblank_in(void);
extern void nes_ppu_vblank_out(void);
extern int nes_ppu_vblnmi_enabled(void);

extern void nes_ppu_set_tblmirrors(int n1, int n2, int n3, int n4);
extern void nes_ppu_set_patterntbl(int tbl, uint8 *ptr, int rom);

extern void nes_ppu_execute(int line, int skip);
extern void nes_ppu_set_mmc2(void (*cb)(uint16 pn));

extern int nes_ppu_init(void);
extern int nes_ppu_reset(void);
extern int nes_ppu_shutdown(void);

extern int nes_ppu_write_context(FILE *fp);
extern int nes_ppu_read_context(const uint8 *buf);

ENDCLINK

#endif /* !NESPPU_H */
