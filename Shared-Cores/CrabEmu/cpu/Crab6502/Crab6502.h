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

#ifndef CRAB6502_H
#define CRAB6502_H

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

typedef union {
    struct {
#ifdef __BIG_ENDIAN__
        uint8 h;
        uint8 l;
#else
        uint8 l;
        uint8 h;
#endif
    } b;
    uint8 bytes[2];
    uint16 w;
} Crab6502_pc_t;

typedef struct Crab6502_struct Crab6502_t;

struct Crab6502_struct {
    Crab6502_pc_t pc;
    uint8 a;
    uint8 x;
    uint8 y;
    uint8 p;
    uint8 s;

    uint8 (*mread)(void *, uint16);
    void (*mwrite)(void *, uint16, uint8);

    uint8 *readmap[256];

    uint8 cli;
    uint8 irq_pending;

    int cycles_in;
    int cycles_burned;
    int cycles_run;
};

/* Flag definitions */
#define CRAB6502_CF     0
#define CRAB6502_ZF     1
#define CRAB6502_IF     2
#define CRAB6502_DF     3
#define CRAB6502_BF     4
#define CRAB6502_RF     5
#define CRAB6502_VF     6
#define CRAB6502_NF     7

/* Flag setting macros */
#define CRAB6502_SET_FLAG(cpu, n)   (cpu)->p |= (1 << (n))
#define CRAB6502_CLEAR_FLAG(cpu, n) (cpu)->p &= (~(1 << (n)))
#define CRAB6502_GET_FLAG(cpu, n)   ((cpu)->p >> (n)) & 1

/* Function definitions */
void Crab6502_init(Crab6502_t *cpu);
void Crab6502_reset(Crab6502_t *cpu);

int Crab6502_execute(Crab6502_t *cpu, int cycles);

void Crab6502_pulse_nmi(Crab6502_t *cpu);
void Crab6502_assert_irq(Crab6502_t *cpu);
void Crab6502_clear_irq(Crab6502_t *cpu);

void Crab6502_release_cycles(Crab6502_t *cpu);

void Crab6502_set_memread(Crab6502_t *cpu,
                          uint8 (*mread)(void *cpu, uint16 addr));
void Crab6502_set_memwrite(Crab6502_t *cpu,
                           void (*mwrite)(void *cpu, uint16 addr, uint8 data));

void Crab6502_set_readmap(Crab6502_t *cpu, uint8 *readmap[256]);

ENDCLINK

#endif /* !CRAB6502_H */
