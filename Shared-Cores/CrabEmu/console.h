/*
    This file is part of CrabEmu.

    Copyright (C) 2014, 2015, 2016 Lawrence Sebald

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

#ifndef CONSOLE_H
#define CONSOLE_H

#include "CrabEmu.h"

CLINKAGE

/* All of the consoles that are supported currently. These may eventually go
   away if there is ever a push to abstract this even more... */
#define CONSOLE_NULL            0   /* No console */
#define CONSOLE_SMS             1   /* Sega Master System or Mark III */
#define CONSOLE_GG              2   /* Sega Game Gear */
#define CONSOLE_SG1000          3   /* Sega SG-1000 */
#define CONSOLE_SC3000          4   /* Sega SC-3000 */
#define CONSOLE_COLECOVISION    5   /* ColecoVision */
#define CONSOLE_NES             6   /* Nintendo Entertainment System */
                                    /* 7 left blank for now... */
#define CONSOLE_CHIP8           8   /* Chip-8 "Console" */

/* Region codes. */
#define REGION_NONE             0x00
#define REGION_JAPAN            0x01
#define REGION_US               0x02
#define REGION_EUROPE           0x04

/* This structure is meant to provide an abstraction of a console, such that
   there can be less console-specific code up at the GUI level. Rignt now, this
   doesn't take care of everything, but that might eventually change... Each
   console should create and initialize a "subclass" of one of these (provide
   a structure such that a console_t is the first member thereof). */
typedef struct crabemu_console {
    int console_family;
    int console_type;
    int initialized;

    /* Initialization-related stuff */
    int (*shutdown)(void);
    int (*reset)(void);
    int (*soft_reset)(void);

    /* Run one frame of output. */
    void (*frame)(int skip);

    /* Save state management */
    int (*load_state)(const char *fn);
    int (*save_state)(const char *fn);
    int (*save_sram)(const char *fn);

    /* Input */
    void (*button_pressed)(int player, int button);
    void (*button_released)(int player, int button);

    /* Video-related functionaity */
    void *(*framebuffer)(void);
    void (*frame_size)(uint32_t *x, uint32_t *y);
    void (*active_size)(uint32_t *x, uint32_t *y, uint32_t *w, uint32_t *h);

    /* Cheats */
    int (*save_cheats)(const char *fn);

    /* Debugging related functions. */
    void (*scanline)(void);
    void (*step)(void);
    void (*finish_frame)(void);
    void (*finish_line)(void);
    int (*current_scanline)(void);
    int (*current_cycles)(void);

    void (*set_control)(int player, int control);
} console_t;

ENDCLINK

#endif /* !CONSOLE_H */
