/*
    This file is part of CrabEmu.

    Copyright (C) 2015 Lawrence Sebald

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

#ifndef CHIP8_H
#define CHIP8_H

#include "CrabEmu.h"
#include "console.h"

CLINKAGE

extern int chip8_init(void);
extern int chip8_mem_load_rom(const char *fn);

/* Console definition. */
typedef struct crabemu_chip8 {
    console_t _base;
} chip8_t;

extern chip8_t chip8_cons;

ENDCLINK

#endif /* !CHIP8_H */
