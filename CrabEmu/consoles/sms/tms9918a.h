/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2006, 2007, 2008, 2012, 2014 Lawrence Sebald

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

#ifndef TMS9918A_H
#define TMS9918A_H

#include "CrabEmu.h"
#include "smsvdp.h"

CLINKAGE

/* The hardwired palette when used in a TMS9918 mode */
/* These constants are taken from Sean Young's TMS9918A Documentation */
#ifdef CRABEMU_32BIT_COLOR
static const pixel_t tms9918_pal[16] = {
    0xFF000000, /* Transparent */
    0xFF000000, /* Black */
    0xFF21C842, /* Medium Green */
    0xFF5EDC78, /* Light Green */
    0xFF5455ED, /* Dark Blue */
    0xFF7D76FC, /* Light Blue */
    0xFFD4524D, /* Dark Red */
    0xFF42EBF5, /* Cyan */
    0xFFFC5554, /* Medium Red */
    0xFFFF7978, /* Light Red */
    0xFFD4C154, /* Dark Yellow */
    0xFFE6CE80, /* Light Yellow */
    0xFF21B03B, /* Dark Green */
    0xFFC95BBA, /* Magenta */
    0xFFCCCCCC, /* Gray */
    0xFFFFFFFF, /* White */
};
#else
static const pixel_t tms9918_pal[16] = {
    0x0000, /* Transparent */ 
    0x0000, /* Black */
    0x1328, /* Medium Green */
    0x2F6F, /* Light Green */
    0x295D, /* Dark Blue */
    0x3DDF, /* Light Blue */
    0x6949, /* Dark Red */
    0x23BE, /* Cyan */
    0x7D4A, /* Medium Red */
    0x7DEF, /* Light Red */
    0x6B0A, /* Dark Yellow */
    0x7330, /* Light Yellow */
    0x12C7, /* Dark Green */
    0x6577, /* Magenta */
    0x6739, /* Gray */
    0x7FFF  /* White */
};
#endif

extern void tms9918a_m0_draw_bg(int line, pixel_t *px);
extern void tms9918a_m1_draw_bg(int line, pixel_t *px);
extern void tms9918a_m2_draw_bg(int line, pixel_t *px);
extern void tms9918a_m3_draw_bg(int line, pixel_t *px);
extern void tms9918a_m023_draw_spr(int line, pixel_t *px);
extern void tms9918a_m023_skip_spr(int line);

extern void tms9918a_vdp_data_write(uint8 data);
extern void tms9918a_vdp_ctl_write(uint8 data);
extern uint8 tms9918a_vdp_data_read(void);
extern uint8 tms9918a_vdp_status_read(void);

extern void tms9918a_vdp_activeframe(uint32_t *x, uint32_t *y, uint32_t *w,
                                     uint32_t *h);

ENDCLINK

#endif /* !TMS9918A_H */
