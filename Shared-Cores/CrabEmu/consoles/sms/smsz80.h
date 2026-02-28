/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2006, 2008, 2009 Lawrence Sebald

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

#ifndef SMSZ80_H
#define SMSZ80_H

#include <stdio.h>
#include "CrabEmu.h"
#include "smsmem.h"

CLINKAGE

extern void sms_z80_reset(void);
extern uint32 sms_z80_run(uint32 cycles);
extern void sms_z80_assert_irq(void);
extern void sms_z80_clear_irq(void);
extern void sms_z80_nmi(void);
extern uint16 sms_z80_get_pc(void);
extern uint32 sms_z80_get_cycles(void);

extern void sms_z80_set_mread(uint8 (*mread)(uint16));
extern void sms_z80_set_mwrite(void (*mwrite)(uint16, uint8));
extern void sms_z80_set_pread(uint8 (*pread)(uint16));
extern void sms_z80_set_pwrite(void (*pwrite)(uint16, uint8));
extern void sms_z80_set_mread16(uint16 (*mread)(uint16));
extern void sms_z80_set_mwrite16(void (*mwrite)(uint16, uint16));
extern void sms_z80_set_readmap(uint8 *readmap[256]);

extern int sms_z80_init(void);
extern int sms_z80_shutdown(void);

#define SMS_Z80_REG_B   0x00
#define SMS_Z80_REG_C   0x01
#define SMS_Z80_REG_D   0x02
#define SMS_Z80_REG_E   0x03
#define SMS_Z80_REG_H   0x04
#define SMS_Z80_REG_L   0x05
#define SMS_Z80_REG_F   0x06
#define SMS_Z80_REG_A   0x07
#define SMS_Z80_REG_PC  0x08
#define SMS_Z80_REG_SP  0x09
#define SMS_Z80_REG_IX  0x0A
#define SMS_Z80_REG_IY  0x0B
#define SMS_Z80_REG_BC  0x0C
#define SMS_Z80_REG_DE  0x0D
#define SMS_Z80_REG_HL  0x0E
#define SMS_Z80_REG_AF  0x0F
#define SMS_Z80_REG_R   0x10
#define SMS_Z80_REG_I   0x11
#define SMS_Z80_REG_BCp 0x12
#define SMS_Z80_REG_DEp 0x13
#define SMS_Z80_REG_HLp 0x14
#define SMS_Z80_REG_AFp 0x15

extern uint16 sms_z80_read_reg(int reg);
extern void sms_z80_write_reg(int reg, uint16 value);

extern int sms_z80_write_context(FILE *fp);
extern int sms_z80_read_context(const uint8 *buf);
extern void sms_z80_read_context_v1(FILE *fp);

ENDCLINK

#endif /* !SMSZ80_H */
