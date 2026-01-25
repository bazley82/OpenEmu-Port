/*
    This file is part of CrabEmu.

    Copyright (C) 2012, 2013 Lawrence Sebald

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

#ifndef NES_SOUND_H
#define NES_SOUND_H

#include "CrabEmu.h"
#include <stdio.h>

CLINKAGE

extern void nes_apu_write(uint16 addr, uint8 data);
extern uint8 nes_apu_read(uint16 addr);
extern void nes_apu_execute(int cycles);
extern int nes_apu_init(void);
extern void nes_apu_shutdown(void);
extern void nes_apu_reset(void);

extern int nes_apu_write_context(FILE *fp);
extern int nes_apu_read_context(const uint8 *buf);

ENDCLINK

#endif /* !NES_SOUND_H */
