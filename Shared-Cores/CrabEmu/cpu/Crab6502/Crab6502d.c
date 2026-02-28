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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "Crab6502d.h"

static const char branches[][4] = {
    "BPL", "BMI", "BVC", "BVS", "BCC", "BCS", "BNE", "BEQ"
};

static const char aluops[][4] = {
    "ORA", "AND", "EOR", "ADC", "STA", "LDA", "CMP", "SBC"
};

static const char aluops2[][4] = {
    "SLO", "RLA", "SRE", "RRA", "SAX", "LAX", "DCP", "ISC"
};

static const char aluops3[][4] = {
    "ASL", "ROL", "LSR", "ROR", "STX", "LDX", "DEC", "INC"
};

static const char opsx8[][4] = {
    "PHP", "CLC", "PLP", "SEC", "PHA", "CLI", "PLA", "SEI",
    "DEY", "TYA", "TAY", "CLV", "INY", "CLD", "INX", "SED"
};

uint16 Crab6502_disassemble(char str[], int len, Crab6502_t *cpu, uint16 addr) {
    uint8 inst = cpu->mread(cpu, addr++);
    uint8 tmp, tmp2, tmp3;

    switch(inst) {
        case 0x00:  /* BRK */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     BRK", inst, tmp);
            break;

        case 0x10:  /* BPL rel */
        case 0x30:  /* BMI rel */
        case 0x50:  /* BVC rel */
        case 0x70:  /* BVS rel */
        case 0x90:  /* BCC rel */
        case 0xB0:  /* BCS rel */
        case 0xD0:  /* BNE rel */
        case 0xF0:  /* BEQ rel */
            tmp = cpu->mread(cpu, addr++);
            tmp3 = inst >> 5;
            snprintf(str, len, "%02X %02X     %s $%04X", inst, tmp,
                     branches[tmp3], addr + (int8)tmp);
            break;

        case 0x20:  /* JSR abs */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  JSR $%04X", inst, tmp, tmp2,
                     tmp | (tmp2 << 8));
            break;

        case 0x40:  /* RTI */
            snprintf(str, len, "%02X        RTI", inst);
            break;

        case 0x60:  /* RTS */
            snprintf(str, len, "%02X        RTS", inst);
            break;

        case 0x80:  /* (U) NOP imm */
        case 0x82:  /* (U) NOP imm */
        case 0xC2:  /* (U) NOP imm */
        case 0xE2:  /* (U) NOP imm */
        case 0x89:  /* (U) NOP imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     NOP", inst, tmp);
            break;

        case 0xA0:  /* LDY imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     LDY #$%02X", inst, tmp, tmp);
            break;

        case 0xC0:  /* CPY imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     CPY #$%02X", inst, tmp, tmp);
            break;

        case 0xE0:  /* CPX imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     CPX #$%02X", inst, tmp, tmp);
            break;

        case 0x01:  /* ORA (ind, X) */
        case 0x21:  /* AND (ind, X) */
        case 0x41:  /* EOR (ind, X) */
        case 0x61:  /* ADC (ind, X) */
        case 0x81:  /* STA (ind, X) */
        case 0xA1:  /* LDA (ind, X) */
        case 0xC1:  /* CMP (ind, X) */
        case 0xE1:  /* SBC (ind, X) */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s ($%02X, X)", inst, tmp,
                     aluops[tmp3], tmp);
            break;

        case 0x11:  /* ORA (ind), Y */
        case 0x31:  /* AND (ind), Y */
        case 0x51:  /* EOR (ind), Y */
        case 0x71:  /* ADC (ind), Y */
        case 0x91:  /* STA (ind), Y */
        case 0xB1:  /* LDA (ind), Y */
        case 0xD1:  /* CMP (ind), Y */
        case 0xF1:  /* SBC (ind), Y */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s ($%02X), Y", inst, tmp,
                     aluops[tmp3], tmp);
            break;

        case 0x02:  /* (U) KIL */
        case 0x12:
        case 0x22:
        case 0x32:
        case 0x42:
        case 0x52:
        case 0x62:
        case 0x72:
        case 0x92:
        case 0xB2:
        case 0xD2:
        case 0xF2:
            snprintf(str, len, "%02X        KIL", inst);
            break;

        case 0xA2:  /* LDX imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     LDX #$%02X", inst, tmp, tmp);
            break;

        case 0x03:  /* (U) SLO (ind, X) */
        case 0x23:  /* (U) RLA (ind, X) */
        case 0x43:  /* (U) SRE (ind, X) */
        case 0x63:  /* (U) RRA (ind, X) */
        case 0x83:  /* (U) SAX (ind, X) */
        case 0xA3:  /* (U) LAX (ind, X) */
        case 0xC3:  /* (U) DCP (ind, X) */
        case 0xE3:  /* (U) ISC (ind, X) */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s ($%02X, X)", inst, tmp,
                     aluops2[tmp3], tmp);
            break;

        case 0x13:  /* (U) SLO (ind), Y */
        case 0x33:  /* (U) RLA (ind), Y */
        case 0x53:  /* (U) SRE (ind), Y */
        case 0x73:  /* (U) RRE (ind), Y */
        case 0xB3:  /* (U) LAX (ind), Y */
        case 0xD3:  /* (U) DCP (ind), Y */
        case 0xF3:  /* (U) ISC (ind), Y */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s ($%02X), Y", inst, tmp,
                     aluops2[tmp3], tmp);
            break;

        case 0x93:  /* (U) AHX (ind), Y */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     AHX ($%02X), Y", inst, tmp, tmp);
            break;

        case 0x04:  /* (U) NOP zpg */
        case 0x14:  /* (U) NOP zpg, X */
        case 0x34:  /* (U) NOP zpg, X */
        case 0x44:  /* (U) NOP zpg */
        case 0x54:  /* (U) NOP zpg, X */
        case 0x64:  /* (U) NOP zpg */
        case 0x74:  /* (U) NOP zpg, X */
        case 0xD4:  /* (U) NOP zpg, X */
        case 0xF4:  /* (U) NOP zpg, X */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     NOP", inst, tmp);
            break;

        case 0x24:  /* BIT zpg */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     BIT $%02X", inst, tmp, tmp);
            break;

        case 0x84:  /* STY zpg */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     STY $%02X", inst, tmp, tmp);
            break;

        case 0x94:  /* STY zpg, X */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     STY $%02X, X", inst, tmp, tmp);
            break;

        case 0xA4:  /* LDY zpg */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     LDY $%02X", inst, tmp, tmp);
            break;

        case 0xB4:  /* LDY zpg, X */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     LDY $%02X, X", inst, tmp, tmp);
            break;

        case 0xC4:  /* CPY zpg */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     CPY $%02X", inst, tmp, tmp);
            break;

        case 0xE4:  /* CPX zpg */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     CPX $%02X", inst, tmp, tmp);
            break;

        case 0x05:  /* ORA zpg */
        case 0x25:  /* AND zpg */
        case 0x45:  /* EOR zpg */
        case 0x65:  /* ADC zpg */
        case 0x85:  /* STA zpg */
        case 0xA5:  /* LDA zpg */
        case 0xC5:  /* CMP zpg */
        case 0xE5:  /* SBC zpg */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s $%02X", inst, tmp,
                     aluops[tmp3], tmp);
            break;

        case 0x15:  /* ORA zpg, X */
        case 0x35:  /* AND zpg, X */
        case 0x55:  /* EOR zpg, X */
        case 0x75:  /* ADC zpg, X */
        case 0x95:  /* STA zpg, X */
        case 0xB5:  /* LDA zpg, X */
        case 0xD5:  /* CMP zpg, X */
        case 0xF5:  /* SBC zpg, X */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s $%02X, X", inst, tmp,
                     aluops[tmp3], tmp);
            break;

        case 0x06:  /* ASL zpg */
        case 0x26:  /* ROL zpg */
        case 0x46:  /* LSR zpg */
        case 0x66:  /* ROR zpg */
        case 0x86:  /* STX zpg */
        case 0xA6:  /* LDX zpg */
        case 0xC6:  /* DEC zpg */
        case 0xE6:  /* INC zpg */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s $%02X", inst, tmp,
                     aluops3[tmp3], tmp);
            break;
            
        case 0x16:  /* ASL zpg, X */
        case 0x36:  /* ROL zpg, X */
        case 0x56:  /* LSR zpg, X */
        case 0x76:  /* ROR zpg, X */
        case 0xD6:  /* DEC zpg, X */
        case 0xF6:  /* INC zpg, X */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s $%02X, X", inst, tmp,
                     aluops3[tmp3], tmp);
            break;

        case 0x96:  /* STX zpg, Y */
        case 0xB6:  /* LDX zpg, Y */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s $%02X, Y", inst, tmp,
                     aluops3[tmp3], tmp);
            break;

        case 0x07:  /* (U) SLO zpg */
        case 0x27:  /* (U) RLA zpg */
        case 0x47:  /* (U) SRE zpg */
        case 0x67:  /* (U) RRA zpg */
        case 0x87:  /* (U) SAX zpg */
        case 0xA7:  /* (U) LAX zpg */
        case 0xC7:  /* (U) DCP zpg */
        case 0xE7:  /* (U) ISC zpg */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s $%02X", inst, tmp,
                     aluops2[tmp3], tmp);
            break;

        case 0x17:  /* (U) SLO zpg, X */
        case 0x37:  /* (U) RLA zpg, X */
        case 0x57:  /* (U) SRE zpg, X */
        case 0x77:  /* (U) RRE zpg, X */
        case 0xD7:  /* (U) DCP zpg, X */
        case 0xF7:  /* (U) ISC zpg, X */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s $%02X, X", inst, tmp,
                     aluops2[tmp3], tmp);
            break;

        case 0x97:  /* (U) SAX zpg, Y */
        case 0xB7:  /* (U) LAX zpg, Y */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s $%02X, Y", inst, tmp,
                     aluops2[tmp3], tmp);
            break;

        case 0x08:  /* PHP */
        case 0x18:  /* CLC */
        case 0x28:  /* PLP */
        case 0x38:  /* SEC */
        case 0x48:  /* PHA */
        case 0x58:  /* CLI */
        case 0x68:  /* PLA */
        case 0x78:  /* SEI */
        case 0x88:  /* DEY */
        case 0x98:  /* TYA */
        case 0xA8:  /* TAY */
        case 0xB8:  /* CLV */
        case 0xC8:  /* INY */
        case 0xD8:  /* CLD */
        case 0xE8:  /* INX */
        case 0xF8:  /* SED */
            snprintf(str, len, "%02X        %s", inst, opsx8[inst >> 4]);
            break;

        case 0x09:  /* ORA imm */
        case 0x29:  /* AND imm */
        case 0x49:  /* EOR imm */
        case 0x69:  /* ADC imm */
        case 0xA9:  /* LDA imm */
        case 0xC9:  /* CMP imm */
        case 0xE9:  /* SBC imm */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     %s #$%02X", inst, tmp,
                     aluops[tmp3], tmp);
            break;

        case 0x19:  /* ORA abs, Y */
        case 0x39:  /* AND abs, Y */
        case 0x59:  /* EOR abs, Y */
        case 0x79:  /* ADC abs, Y */
        case 0x99:  /* STA abs, Y */
        case 0xB9:  /* LDA abs, Y */
        case 0xD9:  /* CMP abs, Y */
        case 0xF9:  /* SBC abs, Y */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  %s $%02X%02X, Y", inst, tmp,
                     tmp2, aluops[tmp3], tmp2, tmp);
            break;

        case 0x0A:  /* ASL A */
        case 0x2A:  /* ROL A */
        case 0x4A:  /* LSR A */
        case 0x6A:  /* ROR A */
            tmp3 = inst >> 5;
            snprintf(str, len, "%02X        %s A", inst, aluops3[tmp3]);
            break;

        case 0x8A:  /* TXA */
            snprintf(str, len, "%02X        TXA", inst);
            break;

        case 0x9A:  /* TXS */
            snprintf(str, len, "%02X        TXS", inst);
            break;

        case 0xAA:  /* TAX */
            snprintf(str, len, "%02X        TAX", inst);
            break;

        case 0xBA:  /* TSX */
            snprintf(str, len, "%02X        TSX", inst);
            break;

        case 0xCA:  /* DEX */
            snprintf(str, len, "%02X        DEX", inst);
            break;

        case 0x1A:  /* (U) NOP */
        case 0x3A:  /* (U) NOP */
        case 0x5A:  /* (U) NOP */
        case 0x7A:  /* (U) NOP */
        case 0xDA:  /* (U) NOP */
        case 0xEA:  /* NOP */
        case 0xFA:  /* (U) NOP */
            snprintf(str, len, "%02X        NOP", inst);
            break;

        case 0x0B:  /* (U) ANC imm */
        case 0x2B:  /* (U) ANC imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     ANC #$%02X", inst, tmp, tmp);
            break;

        case 0x4B:  /* (U) ALR imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     ALR #$%02X", inst, tmp, tmp);
            break;

        case 0x6B:  /* (U) ARR imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     ARR #$%02X", inst, tmp, tmp);
            break;

        case 0x8B:  /* (U) XAA imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     XAA #$%02X", inst, tmp, tmp);
            break;

        case 0xAB:  /* (U) LAX imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     LAX #$%02X", inst, tmp, tmp);
            break;

        case 0xCB:  /* (U) AXS imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     AXS #$%02X", inst, tmp, tmp);
            break;

        case 0xEB:  /* (U) SBC imm */
            tmp = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X     SBC #$%02X", inst, tmp, tmp);
            break;
            
        case 0x1B:  /* (U) SLO abs, Y */
        case 0x3B:  /* (U) RLA abs, Y */
        case 0x5B:  /* (U) SRE abs, Y */
        case 0x7B:  /* (U) RRE abs, Y */
        case 0xDB:  /* (U) DCP abs, Y */
        case 0xFB:  /* (U) ISC abs, Y */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  %s $%02X%02X, X", inst, tmp,
                     tmp2, aluops2[tmp3], tmp2, tmp);
            break;
            
        case 0xBB:  /* (U) LAS abs, Y */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  LAS $%02X%02X, Y", inst, tmp,
                     tmp2, tmp2, tmp);
            break;
            
        case 0x9B:  /* (U) TAS abs, Y */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  TAS $%02X%02X, Y", inst, tmp,
                     tmp2, tmp2, tmp);
            break;

        case 0x0C:  /* (U) NOP abs */
        case 0x1C:  /* (U) NOP abs, X */
        case 0x3C:  /* (U) NOP abs, X */
        case 0x5C:  /* (U) NOP abs, X */
        case 0x7C:  /* (U) NOP abs, X */
        case 0xDC:  /* (U) NOP abs, X */
        case 0xFC:  /* (U) NOP abs, X */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  NOP", inst, tmp, tmp2);
            break;

        case 0x4C:  /* JMP abs */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  JMP $%02X%02X", inst, tmp,
                     tmp2, tmp2, tmp);
            break;

        case 0x6C: /* JMP (ind) */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  JMP ($%02X%02X)", inst, tmp,
                     tmp2, tmp2, tmp);
            break;

        case 0x2C:  /* BIT abs */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  BIT $%02X%02X", inst, tmp, tmp2,
                     tmp2, tmp);
            break;
            
        case 0x8C:  /* STY abs */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  STY $%02X%02X", inst, tmp, tmp2,
                     tmp2, tmp);
            break;
            
        case 0x9C:  /* SHY abs, X */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  SHY $%02X%02X, X", inst, tmp,
                     tmp2, tmp2, tmp);
            break;
            
        case 0xAC:  /* LDY abs */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  LDY $%02X%02X", inst, tmp, tmp2,
                     tmp2, tmp);
            break;
            
        case 0xBC: /* LDY abs, X */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  LDY $%02X%02X, X", inst, tmp,
                     tmp2, tmp2, tmp);
            break;

        case 0xCC: /* CPY abs */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  CPY $%02X%02X", inst, tmp, tmp2,
                     tmp2, tmp);
            break;

        case 0xEC: /* CPX abs */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  CPX $%02X%02X", inst, tmp, tmp2,
                     tmp2, tmp);
            break;

        case 0x0D:  /* ORA abs */
        case 0x2D:  /* AND abs */
        case 0x4D:  /* EOR abs */
        case 0x6D:  /* ADC abs */
        case 0x8D:  /* STA abs */
        case 0xAD:  /* LDA abs */
        case 0xCD:  /* CMP abs */
        case 0xED:  /* SBC abs */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  %s $%02X%02X", inst, tmp,
                     tmp2, aluops[tmp3], tmp2, tmp);
            break;

        case 0x1D:  /* ORA abs, Y */
        case 0x3D:  /* AND abs, Y */
        case 0x5D:  /* EOR abs, Y */
        case 0x7D:  /* ADC abs, Y */
        case 0x9D:  /* STA abs, Y */
        case 0xBD:  /* LDA abs, Y */
        case 0xDD:  /* CMP abs, Y */
        case 0xFD:  /* SBC abs, Y */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  %s $%02X%02X, X", inst, tmp,
                     tmp2, aluops[tmp3], tmp2, tmp);
            break;

        case 0x0E:  /* ASL abs */
        case 0x2E:  /* ROL abs */
        case 0x4E:  /* LSR abs */
        case 0x6E:  /* ROR abs */
        case 0x8E:  /* STX abs */
        case 0xAE:  /* LDX abs */
        case 0xCE:  /* DEC abs */
        case 0xEE:  /* INC abs */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  %s $%02X%02X", inst, tmp, tmp2,
                     aluops3[tmp3], tmp2, tmp);
            break;

        case 0x1E:  /* ASL abs, X */
        case 0x3E:  /* ROL abs, X */
        case 0x5E:  /* LSR abs, X */
        case 0x7E:  /* ROR abs, X */
        case 0xDE:  /* DEC abs, X */
        case 0xFE:  /* INC abs, X */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  %s $%02X%02X, X", inst, tmp,
                     tmp2, aluops3[tmp3], tmp2, tmp);
            break;

        case 0xBE:  /* LDX abs, X */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  LDX $%02X%02X, Y", inst, tmp,
                     tmp2, tmp2, tmp);
            break;

        case 0x9E:  /* (U) SHX abs, Y */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  SHX $%02X%02X, Y", inst, tmp,
                     tmp2, tmp2, tmp);
            break;

        case 0x0F:  /* (U) SLO abs */
        case 0x2F:  /* (U) RLA abs */
        case 0x4F:  /* (U) SRE abs */
        case 0x6F:  /* (U) RRA abs */
        case 0x8F:  /* (U) SAX abs */
        case 0xAF:  /* (U) LAX abs */
        case 0xCF:  /* (U) DCP abs */
        case 0xEF:  /* (U) ISC abs */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  %s $%02X%02X", inst, tmp, tmp2,
                     aluops2[tmp3], tmp2, tmp);
            break;

        case 0x1F:  /* (U) SLO abs, X */
        case 0x3F:  /* (U) RLA abs, X */
        case 0x5F:  /* (U) SRE abs, X */
        case 0x7F:  /* (U) RRE abs, X */
        case 0xDF:  /* (U) DCP abs, X */
        case 0xFF:  /* (U) ISC abs, X */
            tmp3 = inst >> 5;
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  %s $%02X%02X, X", inst, tmp,
                     tmp2, aluops2[tmp3], tmp2, tmp);
            break;

        case 0xBF:  /* (U) LAX abs, Y */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  LAX $%02X%02X, Y", inst, tmp,
                     tmp2, tmp2, tmp);
            break;

        case 0x9F:  /* (U) AHX abs, Y */
            tmp = cpu->mread(cpu, addr++);
            tmp2 = cpu->mread(cpu, addr++);
            snprintf(str, len, "%02X %02X %02X  AHX $%02X%02X, Y", inst, tmp,
                     tmp2, tmp2, tmp);
            break;

        default:
            assert(0);
    }

    return addr;
}

