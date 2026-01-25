/*
    This file is part of CrabEmu.

    Copyright (C) 2009, 2014 Lawrence Sebald

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

#include <stdio.h>
#include <stdlib.h>
#include "smsz80.h"
#include "CrabZ80.h"
#include "cz80.h"

/* Some other portions of the code expect this pointer to exist. I really should
   clean that up sometime... */
CrabZ80_t *cpuz80 = NULL;

/* This Z80 Interface file is a bit of a curiosity. It was written to use
   Stéphane Dallongeville's CZ80 (from which, I took many cues in the many
   rewrites of CrabZ80) solely for the purpose of running it through zexall.
   The code is left here for historical record. */

static uint8 (*pread)(uint16 port);
static void (*pwrite)(uint16 port, uint8 val);
static uint8 (*mread)(uint16 addr);
static void (*mwrite)(uint16 addr, uint8 val);
static uint16 (*mread16)(uint16 addr);
static void (*mwrite16)(uint16 addr, uint16 val);

static u32 FASTCALL cz80_port_read(u32 adr) {
    return (u32)pread((uint16)adr);
}

static void FASTCALL cz80_port_write(u32 adr, u32 data) {
    pwrite((uint16)adr, (uint8)data);
}

static u32 FASTCALL cz80_mem_read(u32 adr) {
    return (u32)mread((uint16)adr);
}

static void FASTCALL cz80_mem_write(u32 adr, u32 data) {
    mwrite((uint16)adr, (uint8)data);
}

static u32 FASTCALL cz80_mem_read16(u32 adr) {
    return (u32)mread16((uint16)adr);
}

static void FASTCALL cz80_mem_write16(u32 adr, u32 data) {
    mwrite16((uint16)adr, (uint16)data);
}

int sms_z80_init(void) {
    Cz80_Init(&CZ80);
    Cz80_Reset(&CZ80);

    Cz80_Set_INPort(&CZ80, &cz80_port_read);
    Cz80_Set_OUTPort(&CZ80, &cz80_port_write);

    Cz80_Set_ReadB(&CZ80, &cz80_mem_read);
    Cz80_Set_WriteB(&CZ80, &cz80_mem_write);
    Cz80_Set_ReadW(&CZ80, &cz80_mem_read16);
    Cz80_Set_WriteW(&CZ80, &cz80_mem_write16);

    return 0;
}

int sms_z80_shutdown(void) {
    return 0;
}

void sms_z80_reset(void) {
    Cz80_Reset(&CZ80);
}

void sms_z80_assert_irq(void) {
    Cz80_Set_IRQ(&CZ80, 0);
}

void sms_z80_clear_irq(void) {
    Cz80_Clear_IRQ(&CZ80);
}

void sms_z80_nmi(void) {
    Cz80_Set_NMI(&CZ80);
}

uint16 sms_z80_get_pc(void) {
    return (uint16)Cz80_Get_PC(&CZ80);
}

uint32 sms_z80_get_cycles(void) {
    return (uint32)Cz80_Get_CycleRemaining(&CZ80);
}

void sms_z80_set_mread(uint8 (*m)(uint16)) {
    mread = m;
}

void sms_z80_set_readmap(uint8 *readmap[256]) {
    int i;

    for(i = 0; i < 256; ++i) {
        Cz80_Set_Fetch(&CZ80, i << 8, ((i + 1) << 8) - 1, (u32)readmap[i]);
    }

    /* This needs to be called after updating the fetch areas, otherwise if
       we've actually changed anything, the PC will be pointing in the wrong
       place, possibly. */
    Cz80_Set_PC(&CZ80, Cz80_Get_PC(&CZ80));
}

void sms_z80_set_mwrite(void (*m)(uint16, uint8)) {
    mwrite = m;
}

void sms_z80_set_pread(uint8 (*p)(uint16)) {
    pread = p;
}

void sms_z80_set_pwrite(void (*p)(uint16, uint8)) {
    pwrite = p;
}

void sms_z80_set_mread16(uint16 (*m)(uint16)) {
    mread16 = m;
}

void sms_z80_set_mwrite16(void (*m)(uint16, uint16)) {
    mwrite16 = m;
}

uint32 sms_z80_run(uint32 cycles) {
    return Cz80_Exec(&CZ80, cycles);
}

uint16 sms_z80_read_reg(int reg) {
    switch(reg) {
        case SMS_Z80_REG_B:
            return CZ80.BC.B.H;
        case SMS_Z80_REG_C:
            return CZ80.BC.B.L;
        case SMS_Z80_REG_D:
            return CZ80.DE.B.H;
        case SMS_Z80_REG_E:
            return CZ80.DE.B.L;
        case SMS_Z80_REG_H:
            return CZ80.HL.B.H;
        case SMS_Z80_REG_L:
            return CZ80.HL.B.L;
        case SMS_Z80_REG_F:
            return CZ80.FA.B.H;
        case SMS_Z80_REG_A:
            return CZ80.FA.B.L;
        case SMS_Z80_REG_PC:
            return (CZ80.PC - CZ80.BasePC) & 0xFFFF;
        case SMS_Z80_REG_SP:
            return CZ80.SP.W;
        case SMS_Z80_REG_IX:
            return CZ80.IX.W;
        case SMS_Z80_REG_IY:
            return CZ80.IY.W;
        case SMS_Z80_REG_BC:
            return CZ80.BC.W;
        case SMS_Z80_REG_DE:
            return CZ80.DE.W;
        case SMS_Z80_REG_HL:
            return CZ80.HL.W;
        case SMS_Z80_REG_AF:
            return CZ80.FA.B.H | (CZ80.FA.B.L << 8);
        case SMS_Z80_REG_R:
            return CZ80.R.B.L;
        case SMS_Z80_REG_I:
            return CZ80.I;
        case SMS_Z80_REG_BCp:
            return CZ80.BC2.W;
        case SMS_Z80_REG_DEp:
            return CZ80.DE2.W;
        case SMS_Z80_REG_HLp:
            return CZ80.HL2.W;
        case SMS_Z80_REG_AFp:
            return CZ80.FA2.B.H | (CZ80.FA2.B.L << 8);
        default:
            return 0xFFFF;
    }
}

void sms_z80_write_reg(int reg, uint16 value) {
    switch(reg) {
        case SMS_Z80_REG_B:
            CZ80.BC.B.H = (uint8)value;
            break;
        case SMS_Z80_REG_C:
            CZ80.BC.B.L = (uint8)value;
            break;
        case SMS_Z80_REG_D:
            CZ80.DE.B.H = (uint8)value;
            break;
        case SMS_Z80_REG_E:
            CZ80.DE.B.L = (uint8)value;
            break;
        case SMS_Z80_REG_H:
            CZ80.HL.B.H = (uint8)value;
            break;
        case SMS_Z80_REG_L:
            CZ80.HL.B.L = (uint8)value;
            break;
        case SMS_Z80_REG_F:
            CZ80.FA.B.H = (uint8)value;
            break;
        case SMS_Z80_REG_A:
            CZ80.FA.B.L = (uint8)value;
            break;
        case SMS_Z80_REG_PC:
            Cz80_Set_PC(&CZ80, value);
            break;
        case SMS_Z80_REG_SP:
            CZ80.SP.W = value;
            break;
        case SMS_Z80_REG_IX:
            CZ80.IX.W = value;
            break;
        case SMS_Z80_REG_IY:
            CZ80.IY.W = value;
            break;
        case SMS_Z80_REG_BC:
            CZ80.BC.W = value;
            break;
        case SMS_Z80_REG_DE:
            CZ80.DE.W = value;
            break;
        case SMS_Z80_REG_HL:
            CZ80.HL.W = value;
            break;
        case SMS_Z80_REG_AF:
            CZ80.FA.W = (value >> 8) | ((value & 0xFF) << 8);
            break;
        case SMS_Z80_REG_R:
            CZ80.R.B.L = (uint8)value;
            break;
        case SMS_Z80_REG_I:
            CZ80.I = (uint8)value;
            break;
        case SMS_Z80_REG_BCp:
            CZ80.BC2.W = value;
            break;
        case SMS_Z80_REG_DEp:
            CZ80.DE2.W = value;
            break;
        case SMS_Z80_REG_HLp:
            CZ80.HL2.W = value;
            break;
        case SMS_Z80_REG_AFp:
            CZ80.FA2.W = (value >> 8) | ((value & 0xFF) << 8);
            break;
    }
}

int sms_z80_write_context(FILE *fp __UNUSED__) {
    /* XXXX */
    return -1;
}

void sms_z80_read_context_v1(FILE *fp __UNUSED__) {
    /* XXXX */
}

int sms_z80_read_context(const uint8 *buf __UNUSED__) {
    /* XXXX */
    return -1;
}
