/*
    This file is part of CrabEmu.

    Copyright (C) 2012 Lawrence Sebald

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

#ifndef CRAB6502D_H
#define CRAB6502D_H

#include "Crab6502.h"

CLINKAGE

uint16 Crab6502_disassemble(char str[], int len, Crab6502_t *cpu, uint16 addr);

ENDCLINK

#endif /* !CRAB6502D_H */
