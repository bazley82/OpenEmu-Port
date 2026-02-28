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

#include <stdlib.h>
#include <string.h>
#include "chip8cpu.h"

static uint8 Chip8CPU_dummy_read(void *cpu, uint16 addr) {
    (void)cpu;
    (void)addr;
    return 0;
}

static uint16 Chip8CPU_dummy_read16(void *cpu, uint16 addr) {
    Chip8CPU_t *c = (Chip8CPU_t *)cpu;

    return (c->mread(cpu, addr) << 8) | c->mread(cpu, addr + 1);
}

static void Chip8CPU_dummy_write(void *cpu, uint16 addr, uint8 data) {
    (void)cpu;
    (void)addr;
    (void)data;
}

static uint8 Chip8CPU_dummy_rread(void *cpu, uint8 reg) {
    (void)cpu;
    (void)reg;
    return 0;
}

static void Chip8CPU_dummy_rwrite(void *cpu, uint8 reg, uint8 data) {
    (void)cpu;
    (void)reg;
    (void)data;
}

static void Chip8CPU_dummy_clear(void *cpu) {
    (void)cpu;
}

static int Chip8CPU_dummy_draw(void *cpu, uint16 a, uint8 x, uint8 y, uint8 h) {
    (void)cpu;
    (void)a;
    (void)x;
    (void)y;
    (void)h;

    return 0;
}

void Chip8CPU_set_memread(Chip8CPU_t *cpu, uint8 (*mread)(void *, uint16)) {
    if(mread == NULL)
        cpu->mread = &Chip8CPU_dummy_read;
    else
        cpu->mread = mread;
}

void Chip8CPU_set_memread16(Chip8CPU_t *cpu, uint16 (*mread)(void *, uint16)) {
    if(mread == NULL)
        cpu->mread16 = &Chip8CPU_dummy_read16;
    else
        cpu->mread16 = mread;
}

void Chip8CPU_set_memwrite(Chip8CPU_t *cpu,
                           void (*mwrite)(void *, uint16, uint8)) {
    if(mwrite == NULL)
        cpu->mwrite = &Chip8CPU_dummy_write;
    else
        cpu->mwrite = mwrite;
}

void Chip8CPU_set_regread(Chip8CPU_t *cpu, uint8 (*rread)(void *, uint8)) {
    if(rread == NULL)
        cpu->rread = &Chip8CPU_dummy_rread;
    else
        cpu->rread = rread;
}

void Chip8CPU_set_regwrite(Chip8CPU_t *cpu,
                           void (*rwrite)(void *, uint8, uint8)) {
    if(rwrite == NULL)
        cpu->rwrite = &Chip8CPU_dummy_rwrite;
    else
        cpu->rwrite = rwrite;
}

void Chip8CPU_set_clearfb(Chip8CPU_t *cpu,
                          void (*clearfb)(void *)) {
    if(clearfb == NULL)
        cpu->clear_fb = &Chip8CPU_dummy_clear;
    else
        cpu->clear_fb = clearfb;
}

void Chip8CPU_set_drawspr(Chip8CPU_t *cpu,
                          int (*drawspr)(void *, uint16, uint8, uint8, uint8)) {
    if(drawspr == NULL)
        cpu->draw_spr = &Chip8CPU_dummy_draw;
    else
        cpu->draw_spr = drawspr;
}

void Chip8CPU_init(Chip8CPU_t *cpu) {
    cpu->mread = &Chip8CPU_dummy_read;
    cpu->mwrite = &Chip8CPU_dummy_write;
    cpu->mread16 = &Chip8CPU_dummy_read16;
    cpu->rread = &Chip8CPU_dummy_rread;
    cpu->rwrite = &Chip8CPU_dummy_rwrite;
    cpu->clear_fb = &Chip8CPU_dummy_clear;
    cpu->draw_spr = &Chip8CPU_dummy_draw;
}

void Chip8CPU_reset(Chip8CPU_t *cpu) {
    cpu->pc = 0x200;
    cpu->i = 0;
    cpu->sp = 0;
    memset(cpu->stack, 0, sizeof(uint16) * 16);
    memset(cpu->v, 0, sizeof(uint8) * 16);
}

void Chip8CPU_execute(Chip8CPU_t *cpu) {
    uint16 inst;

    inst = cpu->mread16(cpu, cpu->pc);
    cpu->pc += 2;
#define INSIDE_CHIP8CPU_EXECUTE
#include "chip8cpuops.h"
#undef INSIDE_CHIP8CPU_EXECUTE
}
