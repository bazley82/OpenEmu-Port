/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2006, 2007, 2008, 2009 Lawrence Sebald

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

CrabZ80_t *cpuz80 = NULL;

int sms_z80_init(void) {
    cpuz80 = (CrabZ80_t *)malloc(sizeof(CrabZ80_t));

    if(cpuz80 == NULL) {
#ifdef DEBUG
        fprintf(stderr, "Out of memory while initializing Z80\n");
#endif
        return -1;
    }

    CrabZ80_init(cpuz80, CRABZ80_CPU_Z80);
    CrabZ80_reset(cpuz80);

    CrabZ80_set_portwrite(cpuz80, sms_port_write);
    CrabZ80_set_portread(cpuz80, sms_port_read);

    return 0;
}

int sms_z80_shutdown(void) {
    free(cpuz80);

    return 0;
}

void sms_z80_reset(void) {
    CrabZ80_reset(cpuz80);
}

void sms_z80_assert_irq(void) {
    CrabZ80_assert_irq(cpuz80, 0xFFFFFFFF);
}

void sms_z80_clear_irq(void) {
    CrabZ80_clear_irq(cpuz80);
}

void sms_z80_nmi(void) {
    CrabZ80_pulse_nmi(cpuz80);
}

uint16 sms_z80_get_pc(void) {
    return cpuz80->pc.w;
}

uint32 sms_z80_get_cycles(void) {
    return cpuz80->cycles;
}

void sms_z80_set_mread(uint8 (*mread)(uint16)) {
    CrabZ80_set_memread(cpuz80, mread);
}

void sms_z80_set_readmap(uint8 *readmap[256]) {
    CrabZ80_set_readmap(cpuz80, readmap);
}

void sms_z80_set_mwrite(void (*mwrite)(uint16, uint8)) {
    CrabZ80_set_memwrite(cpuz80, mwrite);
}

void sms_z80_set_pread(uint8 (*pread)(uint16)) {
    CrabZ80_set_portread(cpuz80, pread);
}

void sms_z80_set_pwrite(void (*pwrite)(uint16, uint8)) {
    CrabZ80_set_portwrite(cpuz80, pwrite);
}

void sms_z80_set_mread16(uint16 (*mread)(uint16)) {
    CrabZ80_set_memread16(cpuz80, mread);
}

void sms_z80_set_mwrite16(void (*mwrite)(uint16, uint16)) {
    CrabZ80_set_memwrite16(cpuz80, mwrite);
}

uint32 sms_z80_run(uint32 cycles) {
    return CrabZ80_execute(cpuz80, cycles);
}

uint16 sms_z80_read_reg(int reg) {
    switch(reg) {
        case SMS_Z80_REG_B:
            return cpuz80->bc.b.h;
        case SMS_Z80_REG_C:
            return cpuz80->bc.b.l;
        case SMS_Z80_REG_D:
            return cpuz80->de.b.h;
        case SMS_Z80_REG_E:
            return cpuz80->de.b.l;
        case SMS_Z80_REG_H:
            return cpuz80->hl.b.h;
        case SMS_Z80_REG_L:
            return cpuz80->hl.b.l;
        case SMS_Z80_REG_F:
            return cpuz80->af.b.l;
        case SMS_Z80_REG_A:
            return cpuz80->af.b.h;
        case SMS_Z80_REG_PC:
            return cpuz80->pc.w;
        case SMS_Z80_REG_SP:
            return cpuz80->sp.w;
        case SMS_Z80_REG_IX:
            return cpuz80->ix.w;
        case SMS_Z80_REG_IY:
            return cpuz80->iy.w;
        case SMS_Z80_REG_BC:
            return cpuz80->bc.w;
        case SMS_Z80_REG_DE:
            return cpuz80->de.w;
        case SMS_Z80_REG_HL:
            return cpuz80->hl.w;
        case SMS_Z80_REG_AF:
            return cpuz80->af.w;
        case SMS_Z80_REG_R:
            return ((cpuz80->ir.b.l & 0x7F) | cpuz80->r_top);
        case SMS_Z80_REG_I:
            return cpuz80->ir.b.h;
        case SMS_Z80_REG_BCp:
            return cpuz80->bcp.w;
        case SMS_Z80_REG_DEp:
            return cpuz80->dep.w;
        case SMS_Z80_REG_HLp:
            return cpuz80->hlp.w;
        case SMS_Z80_REG_AFp:
            return cpuz80->afp.w;
        default:
            return 0xFFFF;
    }
}

void sms_z80_write_reg(int reg, uint16 value) {
    switch(reg) {
        case SMS_Z80_REG_B:
            cpuz80->bc.b.h = (uint8)value;
            break;
        case SMS_Z80_REG_C:
            cpuz80->bc.b.l = (uint8)value;
            break;
        case SMS_Z80_REG_D:
            cpuz80->de.b.h = (uint8)value;
            break;
        case SMS_Z80_REG_E:
            cpuz80->de.b.l = (uint8)value;
            break;
        case SMS_Z80_REG_H:
            cpuz80->hl.b.h = (uint8)value;
            break;
        case SMS_Z80_REG_L:
            cpuz80->hl.b.l = (uint8)value;
            break;
        case SMS_Z80_REG_F:
            cpuz80->af.b.l = (uint8)value;
            break;
        case SMS_Z80_REG_A:
            cpuz80->af.b.h = (uint8)value;
            break;
        case SMS_Z80_REG_PC:
            cpuz80->pc.w = value;
            break;
        case SMS_Z80_REG_SP:
            cpuz80->sp.w = value;
            break;
        case SMS_Z80_REG_IX:
            cpuz80->ix.w = value;
            break;
        case SMS_Z80_REG_IY:
            cpuz80->iy.w = value;
            break;
        case SMS_Z80_REG_BC:
            cpuz80->bc.w = value;
            break;
        case SMS_Z80_REG_DE:
            cpuz80->de.w = value;
            break;
        case SMS_Z80_REG_HL:
            cpuz80->hl.w = value;
            break;
        case SMS_Z80_REG_AF:
            cpuz80->af.w = value;
            break;
        case SMS_Z80_REG_R:
            cpuz80->ir.b.l = (uint8)value;
            cpuz80->r_top = value & 0x80;
            break;
        case SMS_Z80_REG_I:
            cpuz80->ir.b.h = (uint8)value;
            break;
        case SMS_Z80_REG_BCp:
            cpuz80->bcp.w = value;
            break;
        case SMS_Z80_REG_DEp:
            cpuz80->dep.w = value;
            break;
        case SMS_Z80_REG_HLp:
            cpuz80->hlp.w = value;
            break;
        case SMS_Z80_REG_AFp:
            cpuz80->afp.w = value;
            break;
    }
}

#define WRITE_REG(reg) { \
    fwrite(&cpuz80->reg.b.l, 1, 1, fp); \
    fwrite(&cpuz80->reg.b.h, 1, 1, fp); \
}

#define READ_REG(reg) { \
    fread(&cpuz80->reg.b.l, 1, 1, fp); \
    fread(&cpuz80->reg.b.h, 1, 1, fp); \
}

int sms_z80_write_context(FILE *fp) {
    uint8 data[4];

    if(cpuz80 == NULL)
        return -1;

    data[0] = 'Z';
    data[1] = '8';
    data[2] = '0';
    data[3] = '\0';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(52, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    WRITE_REG(af);
    WRITE_REG(bc);
    WRITE_REG(de);
    WRITE_REG(hl);
    WRITE_REG(ix);
    WRITE_REG(iy);
    WRITE_REG(pc);
    WRITE_REG(sp);
    WRITE_REG(ir);
    WRITE_REG(afp);
    WRITE_REG(bcp);
    WRITE_REG(dep);
    WRITE_REG(hlp);

    fwrite(&cpuz80->internal_reg, 1, 1, fp);
    fwrite(&cpuz80->iff1, 1, 1, fp);
    fwrite(&cpuz80->iff2, 1, 1, fp);
    fwrite(&cpuz80->im, 1, 1, fp);
    fwrite(&cpuz80->halt, 1, 1, fp);
    fwrite(&cpuz80->ei, 1, 1, fp);
    fwrite(&cpuz80->r_top, 1, 1, fp);

    data[0] = data[1] = data[2] = 0;
    fwrite(data, 1, 3, fp);

    return 0;
}

int sms_z80_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 52)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Copy in the registers */
    cpuz80->af.b.l = buf[16];
    cpuz80->af.b.h = buf[17];
    cpuz80->bc.b.l = buf[18];
    cpuz80->bc.b.h = buf[19];
    cpuz80->de.b.l = buf[20];
    cpuz80->de.b.h = buf[21];
    cpuz80->hl.b.l = buf[22];
    cpuz80->hl.b.h = buf[23];
    cpuz80->ix.b.l = buf[24];
    cpuz80->ix.b.h = buf[25];
    cpuz80->iy.b.l = buf[26];
    cpuz80->iy.b.h = buf[27];
    cpuz80->pc.b.l = buf[28];
    cpuz80->pc.b.h = buf[29];
    cpuz80->sp.b.l = buf[30];
    cpuz80->sp.b.h = buf[31];
    cpuz80->ir.b.l = buf[32];
    cpuz80->ir.b.h = buf[33];
    cpuz80->afp.b.l = buf[34];
    cpuz80->afp.b.h = buf[35];
    cpuz80->bcp.b.l = buf[36];
    cpuz80->bcp.b.h = buf[37];
    cpuz80->dep.b.l = buf[38];
    cpuz80->dep.b.h = buf[39];
    cpuz80->hlp.b.l = buf[40];
    cpuz80->hlp.b.h = buf[41];
    cpuz80->internal_reg = buf[42];
    cpuz80->iff1 = buf[43];
    cpuz80->iff2 = buf[44];
    cpuz80->im = buf[45];
    cpuz80->halt = buf[46];
    cpuz80->ei = buf[47];
    cpuz80->r_top = buf[48];

    return 0;
}

void sms_z80_read_context_v1(FILE *fp) {
    if(cpuz80 == NULL)
        return;

    READ_REG(af);
    READ_REG(bc);
    READ_REG(de);
    READ_REG(hl);
    READ_REG(ix);
    READ_REG(iy);
    READ_REG(pc);
    READ_REG(sp);
    READ_REG(ir);
    READ_REG(afp);
    READ_REG(bcp);
    READ_REG(dep);
    READ_REG(hlp);

    fread(&cpuz80->internal_reg, 1, 1, fp);
    fread(&cpuz80->iff1, 1, 1, fp);
    fread(&cpuz80->iff2, 1, 1, fp);
    fread(&cpuz80->im, 1, 1, fp);
    fread(&cpuz80->halt, 1, 1, fp);
    fread(&cpuz80->ei, 1, 1, fp);
    fread(&cpuz80->r_top, 1, 1, fp);
}
