/*
    This file is part of CrabEmu.

    Copyright (C) 2012, 2014 Lawrence Sebald

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

#ifndef INSIDE_CRAB6502_EXECUTE
#error This file can only be compiled inside of Crab6502.c. Do not try to
#error include this file in other files.
#endif

/* Undocumented opcodes are denoted by (U) before the mnemonic for the opcode.
   The names for them are taken from http://www.oxyron.de/html/opcodes02.html */
{
uint8 _tmp;
uint16 _addr;
uint32 _tmp32;

switch(inst) {
    case 0x69:  /* ADC imm */
        FETCH_ARG8(_tmp);
        cycles_done += 2;
        goto ADCOP;

    case 0x65:  /* ADC zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
ADCMOP:
        FETCH_BYTE(_addr, _tmp);
ADCOP:
#ifndef CRAB6502_NO_DECIMAL
        /* Decimal mode is... ugly. But it should work. */
        if(cpu->p & 0x08) {
            /* Calculate the two halves as well as the Z flag. */
            uint8 _al = (cpu->a & 0x0F) + (_tmp & 0x0F) + (cpu->p & 0x01);
            _tmp32 = (cpu->a >> 4) + (_tmp >> 4);
            cpu->p = (cpu->p & 0x3C) |
                 (ZNtable[(uint8)(cpu->a + _tmp + (cpu->p & 0x01))] & 0x02);

            /* Adjust the lower half, if needed, adding the half carry to the
               upper half. */
            if(_al > 9) {
                _al += 6;
                ++_tmp32;
            }

            /* Set the N flag if the top bit of the upper half is set, as well
               as the V flag, if needed. */
            cpu->p |= (_tmp32 & 0x08) << 4 |
                (((_tmp ^ cpu->a ^ 0x80) & (_tmp ^ (_tmp32 << 4)) & 0x80) >> 1);

            /* Adjust the upper half and set the C flag, if needed. */
            if(_tmp32 > 9) {
                _tmp32 += 6;
                cpu->p |= 0x01;
            }

            /* Store the final result. */
            cpu->a = (uint8)((_tmp32 << 4) | (_al & 0x0F));
            break;
        }
#endif
        _tmp32 = cpu->a + _tmp + (cpu->p & 0x01);
        cpu->p = (cpu->p & 0x3C) | ZNtable[(uint8)_tmp32] |
            ((_tmp32 >> 8) & 0x01) |
            (((_tmp ^ cpu->a ^ 0x80) & (_tmp ^ _tmp32) & 0x80) >> 1);
        cpu->a = (uint8)_tmp32;
        break;

    case 0x75:  /* ADC zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 4;
        goto ADCMOP;

    case 0x6D:  /* ADC abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto ADCMOP;

    case 0x7D:  /* ADC abs, X */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->x > 0xFF)
            ++cycles_done;
        _addr += cpu->x;
        cycles_done += 4;
        goto ADCMOP;

    case 0x79:  /* ADC abs, Y */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 4;
        goto ADCMOP;

    case 0x61:  /* ADC (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto ADCMOP;

    case 0x71:  /* ADC (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 5;
        goto ADCMOP;

    case 0x93:  /* (U) AHX (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        _addr += cpu->y;
        _tmp = ((_addr >> 8) + 1) & cpu->a & cpu->x;
        cpu->mwrite(cpu, _addr, _tmp);
        cycles_done += 6;
        break;

    case 0x9F:  /* (U) AHX abs, Y */
        FETCH_ARG16(_addr);
        _addr += cpu->y;
        _tmp = ((_addr >> 8) + 1) & cpu->a & cpu->x;
        cpu->mwrite(cpu, _addr, _tmp);
        cycles_done += 5;
        break;

    case 0x4B:  /* (U) ALR imm */
        FETCH_ARG8(_tmp);
        cpu->a &= _tmp;
        cpu->p = (cpu->p & 0x7C) | (cpu->a & 0x01);
        cpu->a >>= 1;
        cpu->p |= ZNtable[cpu->a];
        cycles_done += 2;
        break;

    case 0x0B:  /* (U) ANC imm */
    case 0x2B:  /* (U) ANC imm */
        FETCH_ARG8(_tmp);
        cpu->a &= _tmp;
        cpu->p = (cpu->p & 0x7C) | (cpu->a >> 7) | ZNtable[cpu->a];
        cycles_done += 2;
        break;

    case 0x29:  /* AND imm */
        FETCH_ARG8(_tmp);
        cycles_done += 2;
        goto ANDOP;

    case 0x25:  /* AND zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
ANDMOP:
        FETCH_BYTE(_addr, _tmp);
ANDOP:
        cpu->a &= _tmp;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->a];
        break;

    case 0x35:  /* AND zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 4;
        goto ANDMOP;

    case 0x2D:  /* AND abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto ANDMOP;

    case 0x3D:  /* AND abs, X */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->x > 0xFF)
            ++cycles_done;
        _addr += cpu->x;
        cycles_done += 4;
        goto ANDMOP;

    case 0x39:  /* AND abs, Y */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 4;
        goto ANDMOP;

    case 0x21:  /* AND (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto ANDMOP;

    case 0x31:  /* AND (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 5;
        goto ANDMOP;

    case 0x6B:  /* (U) ARR imm */
        FETCH_ARG8(_tmp);
#ifndef CRAB6502_NO_DECIMAL
        /* Yup... this makes about... zero sense. */
        if(cpu->p & 0x08) {
            uint8 _al, _ah;

            _tmp32 = _tmp = cpu->a & _tmp;
            _ah = _tmp >> 4;
            _al = _tmp & 0x0F;
            _tmp = (_tmp >> 1) | ((cpu->p & 0x01) << 7);

            /* Set all the flags except for C. */
            cpu->p = (cpu->p & 0x3C) | ZNtable[_tmp] | ((_tmp32 ^ _tmp) & 0x40);

            /* BCD adjust (sorta) the low order bits. */
            if(_al + (_al & 0x01) > 5)
                _tmp = (_tmp & 0xf0) | ((_tmp + 6) & 0x0F);

            /* BCD adjust (sorta) the high order bits, and set C if needed. */
            if(_ah + (_ah & 0x01) > 5) {
                cpu->p |= 0x01;
                _tmp = (_tmp + 0x60);
            }

            cpu->a = _tmp;
            cycles_done += 2;
            break;
        }
#endif
        cpu->a = ((cpu->a & _tmp) >> 1) | (cpu->p << 7);
        cpu->p = (cpu->p & 0x3C) | ((cpu->a >> 6) & 0x01) |
            (((cpu->a << 1) ^ cpu->a) & 0x40) | ZNtable[cpu->a];
        cycles_done += 2;
        break;

    case 0x0A:  /* ASL A */
        cpu->p = (cpu->p & 0x7C) | (cpu->a >> 7);
        cpu->a <<= 1;
        cpu->p |= ZNtable[cpu->a];
        cycles_done += 2;
        break;

    case 0x06:  /* ASL zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
ASLOP:
        FETCH_BYTE(_addr, _tmp);
        cpu->p = (cpu->p & 0x7C) | (_tmp >> 7);
        _tmp <<= 1;
        cpu->p |= ZNtable[_tmp];
        cpu->mwrite(cpu, _addr, _tmp);
        break;

    case 0x16:  /* ASL zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto ASLOP;

    case 0x0E:  /* ASL abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto ASLOP;

    case 0x1E:  /* ASL abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto ASLOP;

    case 0xCB:  /* (U) AXS imm */
        FETCH_ARG8(_tmp);
        inst = cpu->x & cpu->a;
        cpu->x = inst - _tmp;
        cpu->p = (cpu->p & 0x7C) | ZNtable[cpu->x];
        if(inst >= _tmp)
            cpu->p |= 0x01;
        cycles_done += 2;
        break;

    case 0x90:  /* BCC rel */
        FETCH_ARG8(_tmp);
        if(!(cpu->p & 0x01))
            goto branchOP;
        cycles_done += 2;
        break;

    case 0xB0:  /* BCS rel */
        FETCH_ARG8(_tmp);
        if(cpu->p & 0x01)
            goto branchOP;
        cycles_done += 2;
        break;

    case 0xF0:  /* BEQ rel */
        FETCH_ARG8(_tmp);
        if(!(cpu->p & 0x02)) {
            cycles_done += 2;
            break;
        }
branchOP:
        cycles_done += 3;
        _addr = cpu->pc.w + (int8)_tmp;
        if(cpu->pc.b.h != (_addr >> 8))
            ++cycles_done;
        cpu->pc.w = _addr;
        break;

    case 0x24:  /* BIT zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
BITOP:
        FETCH_BYTE(_addr, inst);
        cpu->p = (cpu->p & 0x3D) | (ZNtable[inst & cpu->a] & 0x02) |
            (inst & 0xC0);
        break;

    case 0x2C:  /* BIT abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto BITOP;

    case 0x30:  /* BMI rel */
        FETCH_ARG8(_tmp);
        if(cpu->p & 0x80)
            goto branchOP;
        cycles_done += 2;
        break;

    case 0xD0:  /* BNE rel */
        FETCH_ARG8(_tmp);
        if(!(cpu->p & 0x02))
            goto branchOP;
        cycles_done += 2;
        break;

    case 0x10:  /* BPL rel */
        FETCH_ARG8(_tmp);
        if(!(cpu->p & 0x80))
            goto branchOP;
        cycles_done += 2;
        break;

    case 0x00:  /* BRK */
        ++cpu->pc.w;
        cpu->mwrite(cpu, cpu->s-- | 0x100, cpu->pc.b.h);
        cpu->mwrite(cpu, cpu->s-- | 0x100, cpu->pc.b.l);
        cpu->mwrite(cpu, cpu->s-- | 0x100, cpu->p);
        cpu->p |= 0x04;
        FETCH_ADDR(0xFFFE, cpu->pc.w);
        cycles_done += 7;
        break;

    case 0x50:  /* BVC rel */
        FETCH_ARG8(_tmp);
        if(!(cpu->p & 0x40))
            goto branchOP;
        cycles_done += 2;
        break;

    case 0x70:  /* BVS rel */
        FETCH_ARG8(_tmp);
        if(cpu->p & 0x40)
            goto branchOP;
        cycles_done += 2;
        break;

    case 0x18:  /* CLC */
        cpu->p &= 0xFE;
        cycles_done += 2;
        break;

    case 0xD8:  /* CLD */
        cpu->p &= 0xF7;
        cycles_done += 2;
        break;

    case 0x58:  /* CLI */
        cpu->p &= 0xFB;
        cpu->cli = 1;
        cycles_done += 2;
        break;

    case 0xB8:  /* CLV */
        cpu->p &= 0xBF;
        cycles_done += 2;
        break;

    case 0xC9:  /* CMP imm */
        FETCH_ARG8(_tmp);
        inst = cpu->a;
        cycles_done += 2;
        goto CMPOP;

    case 0xC5:  /* CMP zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
CMPAMOP:
        inst = cpu->a;
CMPMOP:
        FETCH_BYTE(_addr, _tmp);
CMPOP:
        cpu->p = (cpu->p & 0x7C) | ZNtable[(uint8)(inst - _tmp)];
        if(inst >= _tmp)
            cpu->p |= 0x01;
        break;

    case 0xD5:  /* CMP zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 4;
        goto CMPAMOP;

    case 0xCD:  /* CMP abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto CMPAMOP;

    case 0xDD:  /* CMP abs, X */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->x > 0xFF)
            ++cycles_done;
        _addr += cpu->x;
        cycles_done += 4;
        goto CMPAMOP;

    case 0xD9:  /* CMP abs, Y */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 4;
        goto CMPAMOP;

    case 0xC1:  /* CMP (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto CMPAMOP;

    case 0xD1:  /* CMP (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 5;
        goto CMPAMOP;

    case 0xE0:  /* CPX imm */
        FETCH_ARG8(_tmp);
        inst = cpu->x;
        cycles_done += 2;
        goto CMPOP;

    case 0xE4:  /* CPX zpg */
        FETCH_ARG8(_addr);
        inst = cpu->x;
        cycles_done += 3;
        goto CMPMOP;

    case 0xEC:  /* CPX abs */
        FETCH_ARG16(_addr);
        inst = cpu->x;
        cycles_done += 4;
        goto CMPMOP;

    case 0xC0:  /* CPY imm */
        FETCH_ARG8(_tmp);
        inst = cpu->y;
        cycles_done += 2;
        goto CMPOP;

    case 0xC4:  /* CPY zpg */
        FETCH_ARG8(_addr);
        inst = cpu->y;
        cycles_done += 3;
        goto CMPMOP;

    case 0xCC:  /* CPY abs */
        FETCH_ARG16(_addr);
        inst = cpu->y;
        cycles_done += 4;
        goto CMPMOP;

    case 0xC7:  /* (U) DCP zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
DCPMOP:
        FETCH_BYTE(_addr, _tmp);
        --_tmp;
        cpu->p = (cpu->p & 0x7C) | ZNtable[(uint8)(cpu->a - _tmp)];
        if(cpu->a >= _tmp)
            cpu->p |= 0x01;
        cpu->mwrite(cpu, _addr, _tmp);
        break;

    case 0xD7:  /* (U) DCP zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto DCPMOP;

    case 0xCF:  /* (U) DCP abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto DCPMOP;

    case 0xDF:  /* (U) DCP abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto DCPMOP;

    case 0xDB:  /* (U) DCP abs, Y */
        FETCH_ARG16(_addr);
        _addr += cpu->y;
        cycles_done += 7;
        goto DCPMOP;

    case 0xC3:  /* (U) DCP (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 7;
        goto DCPMOP;

    case 0xD3:  /* (U) DCP (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        _addr += cpu->y;
        cycles_done += 7;
        goto DCPMOP;

    case 0xC6:  /* DEC zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
DECOP:
        FETCH_BYTE(_addr, _tmp);
        cpu->p = (cpu->p & 0x7D) | ZNtable[(uint8)(--_tmp)];
        cpu->mwrite(cpu, _addr, _tmp);
        break;

    case 0xD6:  /* DEC zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto DECOP;

    case 0xCE:  /* DEC abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto DECOP;

    case 0xDE:  /* DEC abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto DECOP;

    case 0xCA:  /* DEX */
        cpu->p = (cpu->p & 0x7D) | ZNtable[(uint8)(--cpu->x)];
        cycles_done += 2;
        break;

    case 0x88:  /* DEY */
        cpu->p = (cpu->p & 0x7D) | ZNtable[(uint8)(--cpu->y)];
        cycles_done += 2;
        break;

    case 0x49:  /* EOR imm */
        FETCH_ARG8(_tmp);
        cycles_done += 2;
        goto EOROP;

    case 0x45:  /* EOR zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
EORMOP:
        FETCH_BYTE(_addr, _tmp);
EOROP:
        cpu->a ^= _tmp;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->a];
        break;

    case 0x55:  /* EOR zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 4;
        goto EORMOP;

    case 0x4D:  /* EOR abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto EORMOP;

    case 0x5D:  /* EOR abs, X */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->x > 0xFF)
            ++cycles_done;
        _addr += cpu->x;
        cycles_done += 4;
        goto EORMOP;

    case 0x59:  /* EOR abs, Y */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 4;
        goto EORMOP;

    case 0x41:  /* EOR (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto EORMOP;

    case 0x51:  /* EOR (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
        ++cycles_done;
        _addr += cpu->y;
        cycles_done += 5;
        goto EORMOP;

    case 0xE6:  /* INC zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
INCOP:
        FETCH_BYTE(_addr, _tmp);
        cpu->p = (cpu->p & 0x7D) | ZNtable[(uint8)(++_tmp)];
        cpu->mwrite(cpu, _addr, _tmp);
        break;

    case 0xF6:  /* INC zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto INCOP;

    case 0xEE:  /* INC abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto INCOP;

    case 0xFE:  /* INC abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto INCOP;

    case 0xE8:  /* INX */
        cpu->p = (cpu->p & 0x7D) | ZNtable[(uint8)(++cpu->x)];
        cycles_done += 2;
        break;

    case 0xC8:  /* INY */
        cpu->p = (cpu->p & 0x7D) | ZNtable[(uint8)(++cpu->y)];
        cycles_done += 2;
        break;

    case 0xE7:  /* (U) ISC zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
ISCMOP:
        FETCH_BYTE(_addr, _tmp);
        ++_tmp;
        cpu->mwrite(cpu, _addr, _tmp);
        goto SBCOP;

    case 0xF7:  /* (U) ISC zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto ISCMOP;

    case 0xEF:  /* (U) ISC abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto ISCMOP;

    case 0xFF:  /* (U) ISC abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto ISCMOP;

    case 0xFB:  /* (U) ISC abs, Y */
        FETCH_ARG16(_addr);
        _addr += cpu->y;
        cycles_done += 7;
        goto ISCMOP;

    case 0xE3:  /* (U) ISC (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 7;
        goto ISCMOP;

    case 0xF3:  /* (U) ISC (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        _addr += cpu->y;
        cycles_done += 7;
        goto ISCMOP;

    case 0x4C:  /* JMP abs */
        FETCH_ARG16(_addr);
        cpu->pc.w = _addr;
        cycles_done += 3;
        break;

    case 0x6C:  /* JMP (ind) */
        FETCH_ARG16(_addr);
#ifndef CRAB6502_BUGGY_JMP
        FETCH_ADDR(_addr, cpu->pc.w);
#else
        /* Apparently, the original 6502 had a bug in this instruction...
           Ref: http://www.obelisk.demon.co.uk/6502/reference.html#JMP */
        FETCH_BYTE(_addr, cpu->pc.b.l);
        _addr = (_addr & 0xFF00) | ((_addr + 1) & 0x00FF);
        FETCH_BYTE(_addr, cpu->pc.b.h);
#endif
        cycles_done += 5;
        break;

    case 0x20:  /* JSR */
        FETCH_ARG16(_addr);
        --cpu->pc.w;
        cpu->mwrite(cpu, cpu->s-- | 0x100, cpu->pc.b.h);
        cpu->mwrite(cpu, cpu->s-- | 0x100, cpu->pc.b.l);
        cpu->pc.w = _addr;
        cycles_done += 5;
        break;

    case 0xBB:  /* (U) LAS abs, Y */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        FETCH_BYTE(_addr, _tmp);
        cpu->a = cpu->x = cpu->s = cpu->s & _tmp;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->a];
        cycles_done += 4;
        break;

    case 0xA7:  /* (U) LAX zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
LAXMOP:
        FETCH_BYTE(_addr, cpu->a);
        cpu->x = cpu->a;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->a];
        break;

    case 0xB7:  /* (U) LAX zpg, Y */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->y);
        cycles_done += 4;
        goto LAXMOP;

    case 0xAF:  /* (U) LAX abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto LAXMOP;

    case 0xBF:  /* (U) LAX abs, Y */
        FETCH_ARG16(_addr);
        if((_addr >> 8) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 4;
        goto LAXMOP;

    case 0xA3:  /* (U) LAX (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto LAXMOP;

    case 0xB3:  /* (U) LAX (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        if((_addr >> 8) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 5;
        goto LAXMOP;

    case 0xAB:  /* (U) LAX imm */
        /* Sigh... This one really shouldn't be notated that way... */
        FETCH_ARG8(_tmp);
        /* This one does some odd things in some instances... This is apparently
           how this works on the NES, and makes it so that it passes blargg's
           cpu_test5. */
#ifdef CRAB6502_CPU_TYPE_2A03
        cpu->a = 0xFF & _tmp;
#else
        cpu->a &= _tmp;
#endif
        cpu->x = cpu->a;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->a];
        cycles_done += 2;
        break;

    case 0xA9:  /* LDA imm */
        FETCH_ARG8(cpu->a);
        cycles_done += 2;
        goto LDAOP;

    case 0xA5:  /* LDA zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
LDAMOP:
        FETCH_BYTE(_addr, cpu->a);
LDAOP:
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->a];

        break;

    case 0xB5:  /* LDA zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 4;
        goto LDAMOP;

    case 0xAD:  /* LDA abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto LDAMOP;

    case 0xBD:  /* LDA abs, X */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->x > 0xFF)
            ++cycles_done;
        _addr += cpu->x;
        cycles_done += 4;
        goto LDAMOP;

    case 0xB9:  /* LDA abs, Y */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 4;
        goto LDAMOP;

    case 0xA1:  /* LDA (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto LDAMOP;

    case 0xB1:  /* LDA (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 5;
        goto LDAMOP;

    case 0xA2:  /* LDX imm */
        FETCH_ARG8(cpu->x);
        cycles_done += 2;
        goto LDXOP;

    case 0xA6:  /* LDX zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
LDXMOP:
        FETCH_BYTE(_addr, cpu->x);
LDXOP:
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->x];
        break;

    case 0xB6:  /* LDX zpg, Y */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->y);
        cycles_done += 4;
        goto LDXMOP;

    case 0xAE:  /* LDX abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto LDXMOP;

    case 0xBE:  /* LDX abs, Y */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 4;
        goto LDXMOP;

    case 0xA0:  /* LDY imm */
        FETCH_ARG8(cpu->y);
        cycles_done += 2;
        goto LDYOP;

    case 0xA4:  /* LDY zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
LDYMOP:
        FETCH_BYTE(_addr, cpu->y);
LDYOP:
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->y];
        break;

    case 0xB4:  /* LDY zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 4;
        goto LDYMOP;

    case 0xAC:  /* LDY abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto LDYMOP;

    case 0xBC:  /* LDY abs, X */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->x > 0xFF)
            ++cycles_done;
        _addr += cpu->x;
        cycles_done += 4;
        goto LDYMOP;

    case 0x4A:  /* LSR A */
        cpu->p = (cpu->p & 0x7C) | (cpu->a & 0x01);
        cpu->a >>= 1;
        cpu->p |= ZNtable[cpu->a];
        cycles_done += 2;
        break;

    case 0x46:  /* LSR zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
LSROP:
        FETCH_BYTE(_addr, _tmp);
        cpu->p = (cpu->p & 0x7C) | (_tmp & 0x01);
        _tmp >>= 1;
        cpu->p |= ZNtable[_tmp];
        cpu->mwrite(cpu, _addr, _tmp);
        break;

    case 0x56:  /* LSR zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto LSROP;

    case 0x4E:  /* LSR abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto LSROP;

    case 0x5E:  /* LSR abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto LSROP;

    case 0x1A:  /* (U) NOP */
    case 0x3A:  /* (U) NOP */
    case 0x5A:  /* (U) NOP */
    case 0x7A:  /* (U) NOP */
    case 0xDA:  /* (U) NOP */
    case 0xEA:  /* NOP */
    case 0xFA:  /* (U) NOP */
        cycles_done += 2;
        break;

    case 0x80:  /* (U) NOP imm */
    case 0x82:  /* (U) NOP imm */
    case 0x89:  /* (U) NOP imm */
    case 0xC2:  /* (U) NOP imm */
    case 0xE2:  /* (U) NOP imm */
        FETCH_ARG8(_tmp);
        cycles_done += 2;
        break;

    case 0x04:  /* (U) NOP zpg */
    case 0x44:  /* (U) NOP zpg */
    case 0x64:  /* (U) NOP zpg */
        FETCH_ARG8(_tmp);
        cycles_done += 3;
        break;

    case 0x14:  /* (U) NOP zpg, X */
    case 0x34:  /* (U) NOP zpg, X */
    case 0x54:  /* (U) NOP zpg, X */
    case 0x74:  /* (U) NOP zpg, X */
    case 0xD4:  /* (U) NOP zpg, X */
    case 0xF4:  /* (U) NOP zpg, X */
        FETCH_ARG8(_tmp);
        cycles_done += 4;
        break;

    case 0x0C:  /* (U) NOP abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        break;

    case 0x1C:  /* (U) NOP abs, X */
    case 0x3C:  /* (U) NOP abs, X */
    case 0x5C:  /* (U) NOP abs, X */
    case 0x7C:  /* (U) NOP abs, X */
    case 0xDC:  /* (U) NOP abs, X */
    case 0xFC:  /* (U) NOP abs, X */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->x > 0xFF)
            ++cycles_done;
        cycles_done += 4;
        break;

    case 0x09:  /* ORA imm */
        FETCH_ARG8(_tmp);
        cycles_done += 2;
        goto ORAOP;

    case 0x05:  /* ORA zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
ORAMOP:
        FETCH_BYTE(_addr, _tmp);
ORAOP:
        cpu->a |= _tmp;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->a];
        break;

    case 0x15:  /* ORA zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 4;
        goto ORAMOP;

    case 0x0D:  /* ORA abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto ORAMOP;

    case 0x1D:  /* ORA abs, X */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->x > 0xFF)
            ++cycles_done;
        _addr += cpu->x;
        cycles_done += 4;
        goto ORAMOP;

    case 0x19:  /* ORA abs, Y */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 4;
        goto ORAMOP;

    case 0x01:  /* ORA (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto ORAMOP;

    case 0x11:  /* ORA (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 5;
        goto ORAMOP;

    case 0x48:  /* PHA */
        _tmp = cpu->a;
PUSH:
        cpu->mwrite(cpu, cpu->s-- | 0x100, _tmp);
        cycles_done += 3;
        break;

    case 0x08:  /* PHP */
        _tmp = cpu->p;
        goto PUSH;

    case 0x68:  /* PLA */
        POP_STACK(cpu->a);
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->a];
        cycles_done += 4;
        break;

    case 0x28:  /* PLP */
        POP_STACK(cpu->p);
        cpu->p |= 0x30;
        cpu->cli = !(cpu->p & 0x04);
        cycles_done += 4;
        break;

    case 0x27:  /* (U) RLA zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
RLAOP:
        FETCH_BYTE(_addr, _tmp);
        inst = _tmp;
        _tmp = (_tmp << 1) | (cpu->p & 0x01);
        cpu->a &= _tmp;
        cpu->p = (cpu->p & 0x7C) | (inst >> 7) | ZNtable[cpu->a];
        cpu->mwrite(cpu, _addr, _tmp);
        break;

    case 0x37:  /* (U) RLA zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto RLAOP;

    case 0x2F:  /* (U) RLA abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto RLAOP;

    case 0x3F:  /* (U) RLA abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto RLAOP;

    case 0x3B:  /* (U) RLA abs, Y */
        FETCH_ARG16(_addr);
        _addr += cpu->y;
        cycles_done += 7;
        goto RLAOP;

    case 0x23:  /* (U) RLA (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 8;
        goto RLAOP;

    case 0x33:  /* (U) RLA (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        _addr += cpu->y;
        cycles_done += 8;
        goto RLAOP;

    case 0x2A:  /* ROL A */
        inst = cpu->a;
        cpu->a = (cpu->a << 1) | (cpu->p & 0x01);
        cpu->p = (cpu->p & 0x7C) | (inst >> 7) | ZNtable[cpu->a];
        cycles_done += 2;
        break;

    case 0x26:  /* ROL zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
ROLOP:
        FETCH_BYTE(_addr, _tmp);
        inst = _tmp;
        _tmp = (_tmp << 1) | (cpu->p & 0x01);
        cpu->p = (cpu->p & 0x7C) | (inst >> 7) | ZNtable[_tmp];
        cpu->mwrite(cpu, _addr, _tmp);
        break;

    case 0x36:  /* ROL zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto ROLOP;

    case 0x2E:  /* ROL abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto ROLOP;

    case 0x3E:  /* ROL abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto ROLOP;

    case 0x6A:  /* ROR A */
        inst = cpu->a;
        cpu->a = (cpu->a >> 1) | (cpu->p << 7);
        cpu->p = (cpu->p & 0x7C) | (inst & 0x01) | ZNtable[cpu->a];
        cycles_done += 2;
        break;

    case 0x66:  /* ROR zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
ROROP:
        FETCH_BYTE(_addr, _tmp);
        inst = _tmp;
        _tmp = (_tmp >> 1) | (cpu->p << 7);
        cpu->p = (cpu->p & 0x7C) | (inst & 0x01) | ZNtable[_tmp];
        cpu->mwrite(cpu, _addr, _tmp);
        break;

    case 0x76:  /* ROR zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto ROROP;

    case 0x6E:  /* ROR abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto ROROP;

    case 0x7E:  /* ROR abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto ROROP;

    case 0x67:  /* (U) RRA zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
RRAOP:
        FETCH_BYTE(_addr, _tmp);
        inst = _tmp;
        _tmp = (_tmp >> 1) | (cpu->p << 7);
        cpu->p = (cpu->p & 0xFE) | (inst & 0x01);
        cpu->mwrite(cpu, _addr, _tmp);
        goto ADCOP;

    case 0x77:  /* (U) RRA zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto RRAOP;

    case 0x6F:  /* (U) RRA abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto RRAOP;

    case 0x7F:  /* (U) RRA abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto RRAOP;

    case 0x7B:  /* (U) RRA abs, Y */
        FETCH_ARG16(_addr);
        _addr += cpu->y;
        cycles_done += 7;
        goto RRAOP;

    case 0x63:  /* (U) RRA (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 8;
        goto RRAOP;

    case 0x73:  /* (U) RRA (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        _addr += cpu->y;
        cycles_done += 8;
        goto RRAOP;

    case 0x40:  /* RTI */
        POP_STACK(cpu->p);
        POP_STACK(cpu->pc.b.l);
        POP_STACK(cpu->pc.b.h);
        cpu->p |= 0x30;
        cpu->cli = !(cpu->p & 0x04);
        cycles_done += 6;
        break;

    case 0x60:  /* RTS */
        POP_STACK(cpu->pc.b.l);
        POP_STACK(cpu->pc.b.h);
        ++cpu->pc.w;
        cycles_done += 6;
        break;

    case 0x87:  /* (U) SAX zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
SAXOP:
        cpu->mwrite(cpu, _addr, cpu->a & cpu->x);
        break;

    case 0x97:  /* (U) SAX zpg, Y */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->y);
        cycles_done += 4;
        goto SAXOP;

    case 0x83:  /* (U) SAX (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto SAXOP;

    case 0x8F:  /* (U) SAX abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto SAXOP;

    case 0xE9:  /* SBC imm */
    case 0xEB:  /* (U) SBC imm */
        FETCH_ARG8(_tmp);
        cycles_done += 2;
        goto SBCOP;

    case 0xE5:  /* SBC zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
SBCMOP:
        FETCH_BYTE(_addr, _tmp);
SBCOP:
        /* Flag calculation is the same regardless of whether or not we are in
           decimal mode. */
        _tmp32 = cpu->a - _tmp - ((cpu->p & 0x01) ^ 0x01);
        inst = cpu->p;
        cpu->p = (cpu->p & 0x3C) | ZNtable[(uint8)_tmp32] |
            (((~_tmp32) >> 8) & 0x01) |
            (((_tmp ^ cpu->a) & (cpu->a ^ _tmp32) & 0x80) >> 1);

#ifndef CRAB6502_NO_DECIMAL
        /* If we're in decimal mode, calculate the real value that we want in
           the accumulator after all is said and done. */
        if(cpu->p & 0x08) {
            uint8 _al = (cpu->a & 0x0F) - (_tmp & 0x0F) -
                ((inst & 0x01) ^ 0x01);

            if(_al & 0xF0)
                _al = (_al - 6) | 0x10;

            _tmp = (cpu->a >> 4) - (_tmp >> 4) - ((_al & 0x10) >> 4);
            if(_tmp & 0xF0)
                _tmp -= 6;

            _tmp32 = (_tmp << 4) | (_al & 0x0F);
        }
#endif

        cpu->a = (uint8)_tmp32;
        break;

    case 0xF5:  /* SBC zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 4;
        goto SBCMOP;

    case 0xED:  /* SBC abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto SBCMOP;

    case 0xFD:  /* SBC abs, X */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->x > 0xFF)
            ++cycles_done;
        _addr += cpu->x;
        cycles_done += 4;
        goto SBCMOP;

    case 0xF9:  /* SBC abs, Y */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 4;
        goto SBCMOP;

    case 0xE1:  /* SBC (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto SBCMOP;

    case 0xF1:  /* SBC (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 5;
        goto SBCMOP;

    case 0x38:  /* SEC */
        cpu->p |= 0x01;
        cycles_done += 2;
        break;

    case 0xF8:  /* SED */
        cpu->p |= 0x08;
        cycles_done += 2;
        break;

    case 0x78:  /* SEI */
        cpu->p |= 0x04;
        cycles_done += 2;
        break;

    case 0x9E:  /* (U) SHX abs, Y */
        /* This opcode (and the one below it) are just plain weird. Anyway, this
           makes it pass the blargg's cpu_test5. */
        FETCH_ARG16(_addr);
        if(((_addr + cpu->y) >> 8) == (_addr >> 8)) {
            _addr += cpu->y;
            _tmp = ((_addr >> 8) + 1) & cpu->x;
            cpu->mwrite(cpu, _addr, _tmp);
        }
        cycles_done += 5;
        break;

    case 0x9C:  /* (U) SHY abs, X */
        FETCH_ARG16(_addr);
        if(((_addr + cpu->x) >> 8) == (_addr >> 8)) {
            _addr += cpu->x;
            _tmp = ((_addr >> 8) + 1) & cpu->y;
            cpu->mwrite(cpu, _addr, _tmp);
        }
        cycles_done += 5;
        break;

    case 0x07:  /* (U) SLO zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
SLOMOP:
        FETCH_BYTE(_addr, _tmp);
        cpu->p = (cpu->p & 0x7C) | (_tmp >> 7);
        _tmp <<= 1;
        cpu->mwrite(cpu, _addr, _tmp);
        cpu->a |= _tmp;
        cpu->p |= ZNtable[cpu->a];
        break;

    case 0x17:  /* (U) SLO zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 4;
        goto SLOMOP;

    case 0x0F:  /* (U) SLO abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto SLOMOP;

    case 0x1F:  /* (U) SLO abs, X */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->x > 0xFF)
            ++cycles_done;
        _addr += cpu->x;
        cycles_done += 4;
        goto SLOMOP;

    case 0x1B:  /* (U) SLO abs, Y */
        FETCH_ARG16(_addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 4;
        goto SLOMOP;

    case 0x03:  /* (U) SLO (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto SLOMOP;

    case 0x13:  /* (U) SLO (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        if((_addr & 0xFF) + cpu->y > 0xFF)
            ++cycles_done;
        _addr += cpu->y;
        cycles_done += 5;
        goto SLOMOP;

    case 0x47:  /* (U) SRE zpg */
        FETCH_ARG8(_addr);
        cycles_done += 5;
SREOP:
        FETCH_BYTE(_addr, _tmp);
        inst = _tmp;
        _tmp = _tmp >> 1;
        cpu->a ^= _tmp;
        cpu->p = (cpu->p & 0x7C) | (inst & 0x01) | ZNtable[cpu->a];
        cpu->mwrite(cpu, _addr, _tmp);
        break;

    case 0x57:  /* (U) SRE zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 6;
        goto SREOP;

    case 0x4F:  /* (U) SRE abs */
        FETCH_ARG16(_addr);
        cycles_done += 6;
        goto SREOP;

    case 0x5F:  /* (U) SRE abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 7;
        goto SREOP;

    case 0x5B:  /* (U) SRE abs, Y */
        FETCH_ARG16(_addr);
        _addr += cpu->y;
        cycles_done += 7;
        goto SREOP;

    case 0x43:  /* (U) SRE (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 8;
        goto SREOP;

    case 0x53:  /* (U) SRE (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        _addr += cpu->y;
        cycles_done += 8;
        goto SREOP;

    case 0x85:  /* STA zpg */
        FETCH_ARG8(_addr);
        cycles_done += 3;
STAOP:
        _tmp = cpu->a;
ST_OP:
        cpu->mwrite(cpu, _addr, _tmp);
        break;

    case 0x95:  /* STA zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        cycles_done += 4;
        goto STAOP;

    case 0x8D:  /* STA abs */
        FETCH_ARG16(_addr);
        cycles_done += 4;
        goto STAOP;

    case 0x99:  /* STA abs, Y */
        FETCH_ARG16(_addr)
        _addr += cpu->y;
        cycles_done += 5;
        goto STAOP;

    case 0x9D:  /* STA abs, X */
        FETCH_ARG16(_addr);
        _addr += cpu->x;
        cycles_done += 5;
        goto STAOP;

    case 0x81:  /* STA (ind, X) */
        FETCH_ARG8(inst);
        inst += cpu->x;
        FETCH_ADDR_ZPG(inst, _addr);
        cycles_done += 6;
        goto STAOP;

    case 0x91:  /* STA (ind), Y */
        FETCH_ARG8(inst);
        FETCH_ADDR_ZPG(inst, _addr);
        _addr += cpu->y;
        cycles_done += 6;
        goto STAOP;

    case 0x86:  /* STX zpg */
        FETCH_ARG8(_addr);
        _tmp = cpu->x;
        cycles_done += 3;
        goto ST_OP;

    case 0x96:  /* STX zpg, Y */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->y);
        _tmp = cpu->x;
        cycles_done += 4;
        goto ST_OP;

    case 0x8E:  /* STX abs */
        FETCH_ARG16(_addr);
        _tmp = cpu->x;
        cycles_done += 4;
        goto ST_OP;

    case 0x94:  /* STY zpg, X */
        FETCH_ARG8(_addr);
        _addr = (uint8)(_addr + cpu->x);
        _tmp = cpu->y;
        cycles_done += 4;
        goto ST_OP;

    case 0x8C:  /* STY abs */
        FETCH_ARG16(_addr);
        _tmp = cpu->y;
        cycles_done += 4;
        goto ST_OP;

    case 0x84:  /* STY zpg */
        FETCH_ARG8(_addr);
        _tmp = cpu->y;
        cycles_done += 3;
        goto ST_OP;

    case 0x9B:  /* (U) TAS abs, Y */
        FETCH_ARG16(_addr);
        _addr += cpu->y;
        cpu->s = cpu->a & cpu->x;
        _tmp = cpu->s & ((_addr >> 8) + 1);
        cpu->mwrite(cpu, _addr, _tmp);
        cycles_done += 5;
        break;

    case 0xAA:  /* TAX */
        cpu->x = cpu->a;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->x];
        cycles_done += 2;
        break;

    case 0xA8:  /* TAY */
        cpu->y = cpu->a;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->y];
        cycles_done += 2;
        break;

    case 0xBA:  /* TSX */
        cpu->x = cpu->s;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->x];
        cycles_done += 2;
        break;

    case 0x8A:  /* TXA */
        cpu->a = cpu->x;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->a];
        cycles_done += 2;
        break;

    case 0x9A:  /* TXS */
        cpu->s = cpu->x;
        cycles_done += 2;
        break;

    case 0x98:  /* TYA */
        cpu->a = cpu->y;
        cpu->p = (cpu->p & 0x7D) | ZNtable[cpu->a];
        cycles_done += 2;
        break;

    default:
        fprintf(stderr, "Invalid Opcode at addr %04x: %02x\n", cpu->pc.w - 1,
                inst);
        assert(0);
        break;
}
}
