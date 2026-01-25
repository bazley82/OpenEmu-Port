/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2006, 2007, 2008, 2015, 2016 Lawrence Sebald

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

#ifndef INSIDE_CRABZ80_GBEXECUTE
#error This file can only be compiled inside of CrabZ80.c. Do not try to include
#error this file in other files.
#endif

{
uint32 _value;
uint32 _tmp;

switch(inst) {
    case 0x00:  /* NOP */ //d
    case 0x40:  /* LD B, B */ //d
    case 0x49:  /* LD C, C */ //d
    case 0x52:  /* LD D, D */ //d
    case 0x5B:  /* LD E, E */ //d
    case 0x64:  /* LD H, H */ //d
    case 0x6D:  /* LD L, L */ //d
    case 0x7F:  /* LD A, A */ //d
        cycles_done += 4;
        goto out;

    case 0x01:  /* LD BC, nn */ //d
    case 0x11:  /* LD DE, nn */ //d
    case 0x21:  /* LD HL, nn */ //d
        FETCH_ARG16(_value);
        REG16(inst >> 4) = _value;
        cycles_done += 12;
        goto out;

    case 0x31:  /* LD SP, nn */ //d
        FETCH_ARG16(_value);
        cpu->sp.w = _value;
        cycles_done += 12;
        goto out;

    case 0x02:  /* LD (BC), A */ //d
    case 0x12:  /* LD (DE), A */ //d
        cpu->mwrite(REG16(inst >> 4), cpu->af.b.h);
        cycles_done += 8;
        goto out;

    case 0x03:  /* INC BC */ //d
        ++cpu->bc.w;
        cycles_done += 8;
        goto out;

    case 0x13:  /* INC DE */ //d
        ++cpu->de.w;
        cycles_done += 8;
        goto out;

    case 0x23:  /* INC HL */ //d
        ++cpu->hl.w;
        cycles_done += 8;
        goto out;

    case 0x33:  /* INC SP */ //d
        ++cpu->sp.w;
        cycles_done += 8;
        goto out;

    case 0x04:  /* INC B */ //d
    case 0x0C:  /* INC C */ //d
    case 0x14:  /* INC D */ //d
    case 0x1C:  /* INC E */ //d
    case 0x24:  /* INC H */ //d
    case 0x2C:  /* INC L */ //d
    case 0x3C:  /* INC A */ //d
        _value = REG8(inst >> 3);
        GB_INC8(_value);
        REG8(inst >> 3) = _value;
        cycles_done += 4;
        goto out;

    case 0x34:  /* INC (HL) */ //d
        _value = cpu->mread(cpu->hl.w);
        GB_INC8(_value);
        cpu->mwrite(cpu->hl.w, _value);
        cycles_done += 12;
        goto out;

    case 0x05:  /* DEC B */ //d
    case 0x0D:  /* DEC C */ //d
    case 0x15:  /* DEC D */ //d
    case 0x1D:  /* DEC E */ //d
    case 0x25:  /* DEC H */ //d
    case 0x2D:  /* DEC L */ //d
    case 0x3D:  /* DEC A */ //d
        _value = REG8(inst >> 3);
        GB_DEC8(_value);
        REG8(inst >> 3) = _value;
        cycles_done += 4;
        goto out;

    case 0x35:  /* DEC (HL) */ //d
        _value = cpu->mread(cpu->hl.w);
        GB_DEC8(_value);
        cpu->mwrite(cpu->hl.w, _value);
        cycles_done += 12;
        goto out;

    case 0x06:  /* LD B, n */ //d
    case 0x0E:  /* LD C, n */ //d
    case 0x16:  /* LD D, n */ //d
    case 0x1E:  /* LD E, n */ //d
    case 0x26:  /* LD H, n */ //d
    case 0x2E:  /* LD L, n */ //d
    case 0x3E:  /* LD A, n */ //d
        FETCH_ARG8(_value);
        REG8(inst >> 3) = _value;
        cycles_done += 8;
        goto out;

    case 0x36:  /* LD (HL), n */ //d
        FETCH_ARG8(_value);
        cpu->mwrite(cpu->hl.w, _value);
        cycles_done += 12;
        goto out;

    case 0x07:  /* RLCA */ //d
        GB_RLCA();
        cycles_done += 4;
        goto out;

    case 0x09:  /* ADD HL, BC */ //d
    case 0x19:  /* ADD HL, DE */ //d
    case 0x29:  /* ADD HL, HL */ //d
        _value = REG16(inst >> 4);

ADDHLOP:
        cpu->internal_reg = cpu->hl.b.h;
        GB_ADDHL();
        cycles_done += 8;
        goto out;

    case 0x39:  /* ADD HL, SP */ //d
        _value = cpu->sp.w;
        goto ADDHLOP;

    case 0x0A:  /* LD A, (BC) */ //d
    case 0x1A:  /* LD A, (DE) */ //d
        cpu->af.b.h = cpu->mread(REG16(inst >> 4));
        cycles_done += 8;
        goto out;

    case 0x0B:  /* DEC BC */ //d
        --cpu->bc.w;
        cycles_done += 8;
        goto out;

    case 0x1B:  /* DEC DE */ //d
        --cpu->de.w;
        cycles_done += 8;
        goto out;

    case 0x2B:  /* DEC HL */ //d
        --cpu->hl.w;
        cycles_done += 8;
        goto out;

    case 0x3B:  /* DEC SP */ //d
        --cpu->sp.w;
        cycles_done += 8;
        goto out;

    case 0x0F:  /* RRCA */ //d
        GB_RRCA();
        cycles_done += 4;
        goto out;

    case 0x18:  /* JR e */ //d
JROP:
        FETCH_ARG8(_value);
        cpu->pc.w += (int8)_value;
        cycles_done += 12;
        cpu->internal_reg = cpu->pc.b.h;
        goto out;

    case 0x20:  /* JR NZ, e */ //d
        if(!(cpu->af.b.l & 0x80)) {
            goto JROP;
        }

        ++cpu->pc.w;
        cycles_done += 8;
        goto out;

    case 0x28:  /* JR Z, e */ //d
        if(cpu->af.b.l & 0x80) {
            goto JROP;
        }

        ++cpu->pc.w;
        cycles_done += 8;
        goto out;

    case 0x30:  /* JR NC, e */ //d
        if(!(cpu->af.b.l & 0x10)) {
            goto JROP;
        }

        ++cpu->pc.w;
        cycles_done += 8;
        goto out;

    case 0x38:  /* JR C, e */ //d
        if(cpu->af.b.l & 0x10) {
            goto JROP;
        }

        ++cpu->pc.w;
        cycles_done += 8;
        goto out;

    case 0x17:  /* RLA */ //d
        GB_RLA();
        cycles_done += 4;
        goto out;

    case 0x1F:  /* RRA */ //d
        GB_RRA();
        cycles_done += 4;
        goto out;

    case 0x27:  /* DAA */ //d
    {
        int low = cpu->af.b.h & 0x0F;
        int high = cpu->af.b.h >> 4;
        int cf = cpu->af.b.l & 0x10;
        int hf = cpu->af.b.l & 0x20;
        int nf = cpu->af.b.l & 0x40;

        if(cf) {
            _value = (low < 0x0A && !hf) ? 0x60 : 0x66;
        }
        else {
            if(low < 0x0A) {
                if(high < 0x0A) {
                    _value = (hf) ? 0x06 : 0x00;
                }
                else {
                    _value = (hf) ? 0x66 : 0x60;
                }
            }
            else {
                _value = (high < 0x09) ? 0x06 : 0x66;
            }
        }

        if(nf) {
            cpu->af.b.h -= _value;
        }
        else {
            cpu->af.b.h += _value;
        }

        cpu->af.b.l = ((!cpu->af.b.h) << 7) | (nf);

        if(_value >= 0x60)
            cpu->af.b.l |= 0x01;

        if(nf) {
            if(hf && low < 0x06) {
                cpu->af.b.l |= 0x10;
            }
        }
        else if(low >= 10) {
            cpu->af.b.l |= 0x10;
        }

        cycles_done += 4;
        goto out;
    }

    case 0x2F:  /* CPL */ //d
        GB_CPL();
        cycles_done += 4;
        goto out;

    case 0x37:  /* SCF */ //d
        GB_SCF();
        cycles_done += 4;
        goto out;

    case 0x3F:  /* CCF */ //d
        GB_CCF();
        cycles_done += 4;
        goto out;

    case 0x41:  /* LD B, C */ //d
    case 0x42:  /* LD B, D */ //d
    case 0x43:  /* LD B, E */ //d
    case 0x44:  /* LD B, H */ //d
    case 0x45:  /* LD B, L */ //d
    case 0x47:  /* LD B, A */ //d
    case 0x48:  /* LD C, B */ //d
    case 0x4A:  /* LD C, D */ //d
    case 0x4B:  /* LD C, E */ //d
    case 0x4C:  /* LD C, H */ //d
    case 0x4D:  /* LD C, L */ //d
    case 0x4F:  /* LD C, A */ //d
    case 0x50:  /* LD D, B */ //d
    case 0x51:  /* LD D, C */ //d
    case 0x53:  /* LD D, E */ //d
    case 0x54:  /* LD D, H */ //d
    case 0x55:  /* LD D, L */ //d
    case 0x57:  /* LD D, A */ //d
    case 0x58:  /* LD E, B */ //d
    case 0x59:  /* LD E, C */ //d
    case 0x5A:  /* LD E, D */ //d
    case 0x5C:  /* LD E, H */ //d
    case 0x5D:  /* LD E, L */ //d
    case 0x5F:  /* LD E, A */ //d
    case 0x60:  /* LD H, B */ //d
    case 0x61:  /* LD H, C */ //d
    case 0x62:  /* LD H, D */ //d
    case 0x63:  /* LD H, E */ //d
    case 0x65:  /* LD H, L */ //d
    case 0x67:  /* LD H, A */ //d
    case 0x68:  /* LD L, B */ //d
    case 0x69:  /* LD L, C */ //d
    case 0x6A:  /* LD L, D */ //d
    case 0x6B:  /* LD L, E */ //d
    case 0x6C:  /* LD L, H */ //d
    case 0x6F:  /* LD L, A */ //d
    case 0x78:  /* LD A, B */ //d
    case 0x79:  /* LD A, C */ //d
    case 0x7A:  /* LD A, D */ //d
    case 0x7B:  /* LD A, E */ //d
    case 0x7C:  /* LD A, H */ //d
    case 0x7D:  /* LD A, L */ //d
        REG8(inst >> 3) = REG8(inst);
        cycles_done += 4;
        goto out;

    case 0x46:  /* LD B, (HL) */ //d
    case 0x4E:  /* LD C, (HL) */ //d
    case 0x56:  /* LD D, (HL) */ //d
    case 0x5E:  /* LD E, (HL) */ //d
    case 0x66:  /* LD H, (HL) */ //d
    case 0x6E:  /* LD L, (HL) */ //d
    case 0x7E:  /* LD A, (HL) */ //d
        REG8(inst >> 3) = cpu->mread(cpu->hl.w);
        cycles_done += 8;
        goto out;

    case 0x70:  /* LD (HL), B */ //d
    case 0x71:  /* LD (HL), C */ //d
    case 0x72:  /* LD (HL), D */ //d
    case 0x73:  /* LD (HL), E */ //d
    case 0x74:  /* LD (HL), H */ //d
    case 0x75:  /* LD (HL), L */ //d
    case 0x77:  /* LD (HL), A */ //d
        cpu->mwrite(cpu->hl.w, REG8(inst));
        cycles_done += 8;
        goto out;

    case 0x76:  /* HALT */ //d
        --cpu->pc.w;
        cpu->halt = 1;
        cycles_done += 4;
        goto out;

    case 0x80:  /* ADD A, B */ //d
    case 0x81:  /* ADD A, C */ //d
    case 0x82:  /* ADD A, D */ //d
    case 0x83:  /* ADD A, E */ //d
    case 0x84:  /* ADD A, H */ //d
    case 0x85:  /* ADD A, L */ //d
    case 0x87:  /* ADD A, A */ //d
        _value = REG8(inst);
        cycles_done += 4;

ADDOP:
        GB_ADD();
        goto out;

    case 0x86: /* ADD A, (HL) */ //d
        _value = cpu->mread(cpu->hl.w);
        cycles_done += 8;
        goto ADDOP;

    case 0xC6:  /* ADD A, n */ //d
        FETCH_ARG8(_value);
        cycles_done += 8;
        goto ADDOP;

    case 0x88:  /* ADC A, B */ //d
    case 0x89:  /* ADC A, C */ //d
    case 0x8A:  /* ADC A, D */ //d
    case 0x8B:  /* ADC A, E */ //d
    case 0x8C:  /* ADC A, H */ //d
    case 0x8D:  /* ADC A, L */ //d
    case 0x8F:  /* ADC A, A */ //d
        _value = REG8(inst);
        cycles_done += 4;

ADCOP:
        GB_ADC();
        goto out;

    case 0x8E:  /* ADC A, (HL) */ //d
        _value = cpu->mread(cpu->hl.w);
        cycles_done += 8;
        goto ADCOP;

    case 0xCE:  /* ADC A, n */ //d
        FETCH_ARG8(_value);
        cycles_done += 8;
        goto ADCOP;

    case 0x90:  /* SUB A, B */ //d
    case 0x91:  /* SUB A, C */ //d
    case 0x92:  /* SUB A, D */ //d
    case 0x93:  /* SUB A, E */ //d
    case 0x94:  /* SUB A, H */ //d
    case 0x95:  /* SUB A, L */ //d
    case 0x97:  /* SUB A, A */ //d
        _value = REG8(inst);
        cycles_done += 4;

SUBOP:
        GB_SUB();
        goto out;

    case 0x96:  /* SUB A, (HL) */ //d
        _value = cpu->mread(cpu->hl.w);
        cycles_done += 8;
        goto SUBOP;

    case 0xD6:  /* SUB A, n */ //d
        FETCH_ARG8(_value);
        cycles_done += 8;
        goto SUBOP;

    case 0x98:  /* SBC A, B */ //d
    case 0x99:  /* SBC A, C */ //d
    case 0x9A:  /* SBC A, D */ //d
    case 0x9B:  /* SBC A, E */ //d
    case 0x9C:  /* SBC A, H */ //d
    case 0x9D:  /* SBC A, L */ //d
    case 0x9F:  /* SBC A, A */ //d
        _value = REG8(inst);
        cycles_done += 4;

SBCOP:
        GB_SBC();
        goto out;

    case 0x9E:  /* SBC A, (HL) */ //d
        _value = cpu->mread(cpu->hl.w);
        cycles_done += 8;
        goto SBCOP;

    case 0xDE:  /* SBC A, n */ //d
        FETCH_ARG8(_value);
        cycles_done += 8;
        goto SBCOP;

    case 0xA0:  /* AND A, B */ //d
    case 0xA1:  /* AND A, C */ //d
    case 0xA2:  /* AND A, D */ //d
    case 0xA3:  /* AND A, E */ //d
    case 0xA4:  /* AND A, H */ //d
    case 0xA5:  /* AND A, L */ //d
    case 0xA7:  /* AND A, A */ //d
        _value = REG8(inst);
        cycles_done += 4;

ANDOP:
        GB_AND();
        goto out;

    case 0xA6:  /* AND A, (HL) */ //d
        _value = cpu->mread(cpu->hl.w);
        cycles_done += 8;
        goto ANDOP;

    case 0xE6:  /* AND A, n */ //d
        FETCH_ARG8(_value);
        cycles_done += 8;
        goto ANDOP;

    case 0xA8:  /* XOR A, B */ //d
    case 0xA9:  /* XOR A, C */ //d
    case 0xAA:  /* XOR A, D */ //d
    case 0xAB:  /* XOR A, E */ //d
    case 0xAC:  /* XOR A, H */ //d
    case 0xAD:  /* XOR A, L */ //d
    case 0xAF:  /* XOR A, A */ //d
        _value = REG8(inst);
        cycles_done += 4;

XOROP:
        GB_XOR();
        goto out;

    case 0xAE:  /* XOR A, (HL) */ //d
        _value = cpu->mread(cpu->hl.w);
        cycles_done += 8;
        goto XOROP;

    case 0xEE:  /* XOR A, n */ //d
        FETCH_ARG8(_value);
        cycles_done += 8;
        goto XOROP;

    case 0xB0:  /* OR A, B */ //d
    case 0xB1:  /* OR A, C */ //d
    case 0xB2:  /* OR A, D */ //d
    case 0xB3:  /* OR A, E */ //d
    case 0xB4:  /* OR A, H */ //d
    case 0xB5:  /* OR A, L */ //d
    case 0xB7:  /* OR A, A */ //d
        _value = REG8(inst);
        cycles_done += 4;

OROP:
        GB_OR();
        goto out;

    case 0xB6:  /* OR A, (HL) */ //d
        _value = cpu->mread(cpu->hl.w);
        cycles_done += 8;
        goto OROP;

    case 0xF6:  /* OR A, n */ //d
        FETCH_ARG8(_value);
        cycles_done += 8;
        goto OROP;

    case 0xB8:  /* CP A, B */ //d
    case 0xB9:  /* CP A, C */ //d
    case 0xBA:  /* CP A, D */ //d
    case 0xBB:  /* CP A, E */ //d
    case 0xBC:  /* CP A, H */ //d
    case 0xBD:  /* CP A, L */ //d
    case 0xBF:  /* CP A, A */ //d
        _value = REG8(inst);
        cycles_done += 4;

CPOP:
        GB_CP();
        goto out;

    case 0xBE:  /* CP A, (HL) */ //d
        _value = cpu->mread(cpu->hl.w);
        cycles_done += 8;
        goto CPOP;

    case 0xFE:  /* CP A, n */ //d
        FETCH_ARG8(_value);
        cycles_done += 8;
        goto CPOP;

    case 0xC0:  /* RET NZ */ //d
        if(!(cpu->af.b.l & 0x80)) {
            cycles_done += 4;
            goto RETOP;
        }

        cycles_done += 8;
        goto out;

    case 0xC8:  /* RET Z */ //d
        if(!(cpu->af.b.l & 0x80)) {
            cycles_done += 8;
            goto out;
        }

        cycles_done += 4;
        /* Fall through... */

    case 0xC9:  /* RET */ //d
RETOP:
        cpu->pc.w = cpu->mread16(cpu->sp.w);
        cpu->sp.w += 2;
        cycles_done += 16;
        goto out;

    case 0xD0:  /* RET NC */ //d
        if(!(cpu->af.b.l & 0x10)) {
            cycles_done += 4;
            goto RETOP;
        }

        cycles_done += 8;
        goto out;

    case 0xD8:  /* RET C */ //d
        if(cpu->af.b.l & 0x10) {
            cycles_done += 4;
            goto RETOP;
        }

        cycles_done += 8;
        goto out;

    case 0xC1:  /* POP BC */ //d
    case 0xD1:  /* POP DE */ //d
    case 0xE1:  /* POP HL */ //d
        REG16(inst >> 4) = cpu->mread16(cpu->sp.w);
        cpu->sp.w += 2;
        cycles_done += 12;
        goto out;

    case 0xF1:  /* POP AF */ //d
        cpu->af.b.l = cpu->mread(cpu->sp.w++);
        cpu->af.b.h = cpu->mread(cpu->sp.w++);
        cycles_done += 12;
        goto out;

    case 0xC2:  /* JP NZ, ee */ //d
        if(cpu->af.b.l & 0x80)
            goto out_nocondjump;

        cycles_done += 4;
        /* Fall through... */

    case 0xC3:  /* JP ee */ //d
JPOP:
        FETCH_ARG16(_value);
        cpu->pc.w = _value;
        cycles_done += 16;
        goto out;

    case 0xCA:  /* JP Z, ee */ //d
        if(cpu->af.b.l & 0x80) {
            cycles_done += 4;
            goto JPOP;
        }

        goto out_nocondjump;

    case 0xD2:  /* JP NC, ee */ //d
        if(!(cpu->af.b.l & 0x10)) {
            cycles_done += 4;
            goto JPOP;
        }

        goto out_nocondjump;

    case 0xDA:  /* JP C, ee */ //d
        if(cpu->af.b.l & 0x10) {
            cycles_done += 4;
            goto JPOP;
        }

        goto out_nocondjump;

    case 0xC4:  /* CALL NZ, ee */ //d
        if(!(cpu->af.b.l & 0x80))
            goto CALLOP;

        goto out_nocondjump;

    case 0xCC:  /* CALL Z, ee */ //d
        if(!(cpu->af.b.l & 0x80))
            goto out_nocondjump;

        /* Fall through... */

    case 0xCD:  /* CALL ee */ //d
CALLOP:
        FETCH_ARG16(_value);
        cpu->sp.w -= 2;
        cpu->mwrite16(cpu->sp.w, cpu->pc.w);
        cpu->pc.w = _value;
        cycles_done += 24;
        goto out;

    case 0xD4:  /* CALL NC, ee */ //d
        if(!(cpu->af.b.l & 0x10))
            goto CALLOP;

        goto out_nocondjump;

    case 0xDC:  /* CALL C, ee */ //d
        if(cpu->af.b.l & 0x10)
            goto CALLOP;

        goto out_nocondjump;

    case 0xC5:  /* PUSH BC */ //d
    case 0xD5:  /* PUSH DE */ //d
    case 0xE5:  /* PUSH HL */ //d
        cpu->sp.w -= 2;
        cpu->mwrite16(cpu->sp.w, REG16(inst >> 4));
        cycles_done += 16;
        goto out;

    case 0xF5:  /* PUSH AF */ //d
        cpu->mwrite(--cpu->sp.w, cpu->af.b.h);
        cpu->mwrite(--cpu->sp.w, cpu->af.b.l);
        cycles_done += 16;
        goto out;

    case 0xC7:  /* RST 0h */ //d
    case 0xCF:  /* RST 8h */ //d
    case 0xD7:  /* RST 10h */ //d
    case 0xDF:  /* RST 18h */ //d
    case 0xE7:  /* RST 20h */ //d
    case 0xEF:  /* RST 28h */ //d
    case 0xF7:  /* RST 30h */ //d
    case 0xFF:  /* RST 38h */ //d
        cpu->sp.w -= 2;
        cpu->mwrite16(cpu->sp.w, cpu->pc.w);
        cpu->pc.w = inst & 0x38;
        cycles_done += 16;
        goto out;

    case 0xE9:  /* JP (HL) */ //d
        cpu->pc.w = cpu->hl.w;
        cycles_done += 4;
        goto out;

    case 0xF3:  /* DI */ //d
        cpu->iff1 = 0;
        cycles_done += 4;
        goto out;

    case 0xF9:  /* LD SP, HL */ //d
        cpu->sp.w = cpu->hl.w;
        cycles_done += 8;
        goto out;

    case 0xFB:  /* EI */ //d
        cpu->iff1 = cpu->ei = 1;
        cycles_done += 4;
        goto out;

    /* Weird instructions that don't match Z80. */
    case 0x08: /* LD (nn), SP */ //d
        FETCH_ARG16(_value)
        cpu->mwrite16(_value, cpu->sp.w);
        cycles_done += 20;
        goto out;

    case 0x10: /* STOP */ //d, sorta.
        // XXXX
        --cpu->pc.w;
        cpu->halt = 1;
        cycles_done += 4;
        goto out;

    case 0x22: /* LD (HL+), A */ //d
        cpu->mwrite(cpu->hl.w++, cpu->af.b.h);
        cycles_done += 8;
        goto out;

    case 0x2A: /* LD A, (HL+) */ //d
        cpu->af.b.l = cpu->mread(cpu->hl.w++);
        cycles_done += 8;
        goto out;

    case 0x32: /* LD (HL-), A */ //d
        cpu->mwrite(cpu->hl.w--, cpu->af.b.h);
        cycles_done += 8;
        goto out;

    case 0x3A: /* LD A, (HL-) */ //d
        cpu->af.b.l = cpu->mread(cpu->hl.w--);
        cycles_done += 8;
        goto out;

    case 0xD9:  /* RETI */ //d
        cpu->pc.w = cpu->mread16(cpu->sp.w);
        cpu->sp.w += 2;
        cpu->iff1 = 1;
        cycles_done += 16;
        goto out;

    case 0xE0:  /* LD (0xFF00 + n), A */ //d
        FETCH_ARG8(_value);
        cpu->mwrite(0xFF00 + _value, cpu->af.b.h);
        cycles_done += 12;
        goto out;

    case 0xE2:  /* LD (0xFF00 + C), A */ //d
        cpu->mwrite(0xFF00 + cpu->bc.b.l, cpu->af.b.h);
        cycles_done += 8;
        goto out;

    case 0xE8:  /* ADD SP, n */ //d
        FETCH_ARG8(_value);
        _tmp = cpu->sp.b.l + ((int8)_value);
        /* ... Why? ... */
        cpu->af.b.l = (((cpu->sp.b.l ^ _tmp ^ _value) & 0x10) << 1) |
            ((_tmp >> 4) & 0x10);
        cpu->sp.w += (int8)_value;
        cycles_done += 16;
        goto out;

    case 0xEA:  /* LD (nn), A */ //d
        FETCH_ARG16(_value);
        cpu->mwrite(_value, cpu->af.b.h);
        cycles_done += 16;
        goto out;

    case 0xF0:  /* LD A, (0xFF00 + n) */ //d
        FETCH_ARG8(_value);
        cpu->af.b.h = cpu->mread(0xFF00 + _value);
        cycles_done += 12;
        goto out;

    case 0xF2:  /* LD A, (0xFF00 + C) */ //d
        cpu->af.b.h = cpu->mread(0xFF00 + cpu->bc.b.l);
        cycles_done += 8;
        goto out;

    case 0xF8:  /* LD HL, SP + nn */ //d
        FETCH_ARG8(_value);
        _tmp = cpu->sp.b.l + ((int8)_value);
        /* ... Why? ... */
        cpu->af.b.l = (((cpu->sp.b.l ^ _tmp ^ _value) & 0x10) << 1) |
            ((_tmp >> 4) & 0x10);
        cpu->hl.w = cpu->sp.w + ((int8)_value);
        cycles_done += 12;
        goto out;

    case 0xFA:  /* LD A, (nn) */ //d
        FETCH_ARG16(_value);
        cpu->af.b.h = cpu->mread(_value);
        cycles_done += 16;
        goto out;

    /* Illegal opcodes.... */
    case 0xD3:
    case 0xDB:
    case 0xDD:
    case 0xE3:
    case 0xE4:
    case 0xEB:
    case 0xEC:
    case 0xED:
    case 0xF4:
    case 0xFC:
    case 0xFD:
        /* XXXX */
        goto out;

    case 0xCB:  /* CB-prefix */
        goto execCB;
}

execCB:
#include "CrabZ80gbopsCB.h"
/* We shouldn't get here. */

/* Conditional JP and CALL instructions that don't end up jumping end up
   coming here instead. This falls through back to CrabZ80.c (essentially to the
   same place that goto out would put us). */
out_nocondjump: //d
    cycles_done += 12;
    cpu->pc.w += 2;

    /* Fall through... */
}
