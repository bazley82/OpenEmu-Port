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

#ifndef ROM_H
#define ROM_H

#include "CrabEmu.h"

CLINKAGE

extern int rom_detect_console(const char *fn);
extern uint32 rom_crc32(const uint8 *data, int size);
extern uint32 rom_adler32(const uint8 *buffer, uint32 buflen);

ENDCLINK

#endif /* !ROM_H */
