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

#ifndef CHIP8CPU_H
#define CHIP8CPU_H

#ifdef IN_CRABEMU
#include "CrabEmu.h"
#else

#ifdef __cplusplus
#define CLINKAGE extern "C" {
#define ENDCLINK }
#else
#define CLINKAGE
#define ENDCLINK
#endif /* __cplusplus */

#ifndef CRABEMU_TYPEDEFS
#define CRABEMU_TYPEDEFS

#ifdef _arch_dreamcast
#include <arch/types.h>
#else

#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;
typedef uint32_t int32;
#endif /* _arch_dreamcast */
#endif /* CRABEMU_TYPEDEFS */
#endif /* IN_CRABEMU */

CLINKAGE

typedef struct Chip8CPU_struct Chip8CPU_t;

struct Chip8CPU_struct {
    uint16 pc;
    uint16 i;
    uint16 stack[16];
    uint8 v[16];
    uint8 sp;

    uint8 (*mread)(void *, uint16);
    void (*mwrite)(void *, uint16, uint8);
    uint16 (*mread16)(void *, uint16);
    uint8 (*rread)(void *, uint8);
    void (*rwrite)(void *, uint8, uint8);

    void (*clear_fb)(void *);
    int (*draw_spr)(void *, uint16, uint8, uint8, uint8);
};

/* Key statuses are defined as registers 0x00-0x0F */
#define CHIP8_REG_DT        0x10
#define CHIP8_REG_ST        0x11

/* Function definitions */
void Chip8CPU_init(Chip8CPU_t *cpu);
void Chip8CPU_reset(Chip8CPU_t *cpu);

void Chip8CPU_execute(Chip8CPU_t *cpu);

void Chip8CPU_set_memread(Chip8CPU_t *cpu,
                          uint8 (*mread)(void *cpu, uint16 addr));
void Chip8CPU_set_memread16(Chip8CPU_t *cpu,
                            uint16 (*mread)(void *cpu, uint16 addr));
void Chip8CPU_set_memwrite(Chip8CPU_t *cpu,
                           void (*mwrite)(void *cpu, uint16 addr, uint8 data));

void Chip8CPU_set_regread(Chip8CPU_t *cpu,
                          uint8 (*rread)(void *cpu, uint8 reg));
void Chip8CPU_set_regwrite(Chip8CPU_t *cpu,
                           void (*rwrite)(void *cpu, uint8 reg, uint8 data));

void Chip8CPU_set_clearfb(Chip8CPU_t *cpu, void (*clearfb)(void*));
void Chip8CPU_set_drawspr(Chip8CPU_t *cpu,
                          int (*drawspr)(void *, uint16, uint8, uint8, uint8));

ENDCLINK

#endif /* !CHIP8CPU_H */
