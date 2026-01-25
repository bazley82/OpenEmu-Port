/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2007, 2009, 2012, 2014 Lawrence Sebald

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

#ifndef SMS_H
#define SMS_H

#include "CrabEmu.h"
#include "console.h"

CLINKAGE

#include <stdio.h>

extern int sms_init(int video_system, int region, int borders);
extern int sms_reset(void);
extern int sms_soft_reset(void);
extern int sms_shutdown(void);

extern void sms_button_pressed(int player, int button);
extern void sms_button_released(int player, int button);

extern void sms_set_console(int console);
extern int sms_cycles_elapsed(void);

extern int sms_psg_write_context(FILE *fp);
extern int sms_psg_read_context(const uint8 *buf);

extern int sms_save_state(const char *filename);
extern int sms_load_state(const char *filename);

extern int sms_write_state(FILE *fp);
extern int sms_read_state(FILE *fp);

/* Old button defines. These define the raw bits used for the data. */
#define SMS_PAD1_UP     0x0001
#define SMS_PAD1_DOWN   0x0002
#define SMS_PAD1_LEFT   0x0004
#define SMS_PAD1_RIGHT  0x0008
#define SMS_PAD1_A      0x0010
#define SMS_PAD1_B      0x0020
#define SMS_PAD2_UP     0x0040
#define SMS_PAD2_DOWN   0x0080
#define SMS_PAD2_LEFT   0x0100
#define SMS_PAD2_RIGHT  0x0200
#define SMS_PAD2_A      0x0400
#define SMS_PAD2_B      0x0800
#define SMS_RESET       0x1000
#define GG_START        0xFFFF

/* New button defines. These should be fed into the button parameter of
   sms_button_pressed() and sms_button_released(). */
#define SMS_UP              0
#define SMS_DOWN            1
#define SMS_LEFT            2
#define SMS_RIGHT           3
#define SMS_BUTTON_1        4
#define SMS_BUTTON_2        5
#define GAMEGEAR_START      6
#define SMS_QUIT            7   /* This one is ignored... */
#define SMS_CONSOLE_RESET   8

/* Buttons for the Graphic Board */
#define SMS_GFXBD_1         0
#define SMS_GFXBD_2         1
#define SMS_GFXBD_3         2

/* Control pad types. */
#define SMS_PADTYPE_NONE        0
#define SMS_PADTYPE_CONTROL_PAD 1
#define SMS_PADTYPE_GFX_BOARD   2

#define SMS_PAD1_TL     SMS_PAD1_A
#define SMS_PAD1_TR     SMS_PAD1_B
#define SMS_PAD2_TL     SMS_PAD2_A
#define SMS_PAD2_TR     SMS_PAD2_B
#define SMS_PAD1_TH     0x4000
#define SMS_PAD2_TH     0x8000

#define SMS_TH_MASK     0xC000

/* Region types */
#define SMS_REGION_DOMESTIC REGION_JAPAN
#define SMS_REGION_EXPORT   REGION_US

/* Video Standards */
#define SMS_VIDEO_NTSC VIDEO_NTSC
#define SMS_VIDEO_PAL  VIDEO_PAL

#define SMS_CYCLES_PER_LINE 228

/* Console definition. */
typedef struct crabemu_sms {
    console_t _base;
} sms_t;

extern sms_t sms_cons;

ENDCLINK

#endif /* !SMS_H */
