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

#include "Crab6502.h"
#include "Crab6502_tables.h"
#include "Crab6502_macros.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Define one of the following CPU types to pre-configure various options:
    CRAB6502_CPU_TYPE_NMOS - The original NMOS 6502
    CRAB6502_CPU_TYPE_2A03 - The Ricoh 2A03 (used in the NES)
*/
#if defined(CRAB6502_CPU_TYPE_NMOS)
#   define CRAB6502_BUGGY_JMP      1
#elif defined(CRAB6502_CPU_TYPE_2A03)
#   define CRAB6502_BUGGY_JMP      1
#   define CRAB6502_NO_DECIMAL     1
#endif

static uint8 Crab6502_dummy_read(void *cpu, uint16 addr) {
    (void)cpu;
    (void)addr;
    return 0;
}

static void Crab6502_dummy_write(void *cpu, uint16 addr, uint8 data) {
    (void)cpu;
    (void)addr;
    (void)data;
}

void Crab6502_set_memread(Crab6502_t *cpu, uint8 (*mread)(void *, uint16)) {
    if(mread == NULL)
        cpu->mread = &Crab6502_dummy_read;
    else
        cpu->mread = mread;
}

void Crab6502_set_memwrite(Crab6502_t *cpu,
                           void (*mwrite)(void *, uint16, uint8)) {
    if(mwrite == NULL)
        cpu->mwrite = &Crab6502_dummy_write;
    else
        cpu->mwrite = mwrite;
}

void Crab6502_set_readmap(Crab6502_t *cpu, uint8 *readmap[256]) {
    memcpy(cpu->readmap, readmap, 256 * sizeof(uint8 *));
}

void Crab6502_init(Crab6502_t *cpu) {
    cpu->mread = Crab6502_dummy_read;
    cpu->mwrite = Crab6502_dummy_write;

    memset(cpu->readmap, 0, 256 * sizeof(uint8 *));
}

void Crab6502_reset(Crab6502_t *cpu) {
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->p = 0x34;
    cpu->s = 0xFF;
    cpu->cycles_in = cpu->cycles_burned = 0;
    cpu->cli = cpu->irq_pending = 0;

    /* Read the reset vector... */
    cpu->pc.b.l = cpu->mread(cpu, 0xFFFC);
    cpu->pc.b.h = cpu->mread(cpu, 0xFFFD);
}

static int Crab6502_take_irq(Crab6502_t *cpu, uint16 vector) {
    /* Push our state onto the stack */
    cpu->mwrite(cpu, cpu->s-- | 0x100, cpu->pc.b.h);
    cpu->mwrite(cpu, cpu->s-- | 0x100, cpu->pc.b.l);
    cpu->mwrite(cpu, cpu->s-- | 0x100, cpu->p & 0xEF);

    /* Disable interrupts */
    cpu->p |= 0x04;

    /* Read the vector */
    cpu->pc.b.l = cpu->mread(cpu, vector);
    cpu->pc.b.h = cpu->mread(cpu, vector + 1);

    /* Burn 7 cycles... */
    return 7;
}    

void Crab6502_pulse_nmi(Crab6502_t *cpu) {
    cpu->irq_pending |= 2;
}

void Crab6502_assert_irq(Crab6502_t *cpu) {
    cpu->irq_pending |= 1;
}

void Crab6502_clear_irq(Crab6502_t *cpu) {
    cpu->irq_pending &= 2;
}

void Crab6502_release_cycles(Crab6502_t *cpu) {
    cpu->cycles_in = 0;
}

int Crab6502_execute(Crab6502_t *cpu, int cycles) {
    register int cycles_done = 0;
    uint8 inst;

    cpu->cycles_in = cycles;
    cpu->cycles_run = 0;

    while(cycles_done < cpu->cycles_in) {
        if(cpu->irq_pending & 2) {
            cycles_done += Crab6502_take_irq(cpu, 0xFFFA);
            cpu->irq_pending &= ~2;
        }
        else if(cpu->irq_pending && !cpu->cli && !(cpu->p & 0x04)) {
            cycles_done += Crab6502_take_irq(cpu, 0xFFFE);
        }

        cpu->cli = 0;

        FETCH_ARG8(inst);
#define INSIDE_CRAB6502_EXECUTE
#include "Crab6502ops.h"
#undef INSIDE_CRAB6502_EXECUTE

        cycles_done += cpu->cycles_burned;
        cpu->cycles_burned = 0;
        cpu->cycles_run = cycles_done;
    }

    return cycles_done;
}
