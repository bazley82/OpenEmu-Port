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

#define GB_INC8(val) {   \
    uint32 _tmp = (val) + 1; \
    cpu->af.b.l = ((!_tmp) << 7) | \
        ((_tmp & 0x0F) ? 0x00 : 0x20) | \
        (cpu->af.b.l & 0x10); \
    (val) = _tmp; \
}

#define GB_DEC8(val) {   \
    uint32 _tmp = (val) - 1; \
    cpu->af.b.l = ((!_tmp) << 7) | \
        (cpu->af.b.l & 0x01) | \
        ((((val) ^ _tmp) & 0x10) << 1) | \
        0x40; \
    (val) = _tmp; \
}

#define GB_RLCA() {   \
    cpu->af.b.h = (cpu->af.b.h << 1) | (cpu->af.b.h >> 7); \
    cpu->af.b.l = (cpu->af.b.l & 0xEF) | ((cpu->af.b.h & 0x01) << 4); \
}

#define GB_RRCA() {   \
    cpu->af.b.l = (cpu->af.b.l & 0xEF) | ((cpu->af.b.h & 0x01) << 4); \
    cpu->af.b.h = (cpu->af.b.h >> 1) | (cpu->af.b.h << 7); \
}

#define GB_RLA() {   \
    cpu->af.b.l = (cpu->af.b.l & 0xEF) | ((cpu->af.b.h >> 3) & 0x10); \
    cpu->af.b.h = (cpu->af.b.h << 1) | (cpu->af.b.l & 0x01); \
}

#define GB_RRA() {   \
    cpu->af.b.l = (cpu->af.b.l & 0xEF) | ((cpu->af.b.h & 0x01) << 4); \
    cpu->af.b.h = (cpu->af.b.h >> 1) | (cpu->af.b.l << 7); \
}

#define GB_CPL() {   \
    cpu->af.b.h ^= 0xFF; \
    cpu->af.b.l = (cpu->af.b.l & 0x9F) | 0x60; \
}

#define GB_SCF() {   \
    cpu->af.b.l = (cpu->af.b.l & 0x80) | 0x10; \
}

#define GB_CCF() {   \
    cpu->af.b.l = (cpu->af.b.l & 0x80) | ((cpu->af.b.l ^ 0x10) & 0x10); \
}

#define GB_ADD() {   \
    uint32 _tmp = cpu->af.b.h + _value; \
    cpu->af.b.l = ((!((uint8)_tmp)) << 7) | \
        (((cpu->af.b.h ^ _tmp ^ _value) & 0x10) << 1) | \
        ((_tmp >> 4) & 0x10); \
    cpu->af.b.h = _tmp;  \
}

#define GB_ADC() {   \
    uint32 _tmp = cpu->af.b.h + _value + (cpu->af.b.l & 0x01); \
    cpu->af.b.l = ((!((uint8)_tmp)) << 7) | \
        (((cpu->af.b.h ^ _tmp ^ _value) & 0x10) << 1) | \
        ((_tmp >> 4) & 0x10); \
    cpu->af.b.h = _tmp; \
}

#define GB_SUB() {   \
    uint32 _tmp = cpu->af.b.h - _value; \
    cpu->af.b.l = ((!((uint8)_tmp)) << 7) | \
        (((cpu->af.b.h ^ _tmp ^ _value) & 0x10) << 1) | \
        0x40 | ((_tmp >> 4) & 0x10); \
    cpu->af.b.h = _tmp; \
}

#define GB_SBC() {   \
    uint32 _tmp = cpu->af.b.h - (cpu->af.b.l & 0x01) - _value; \
    cpu->af.b.l = ((!((uint8)_tmp)) << 7) | \
        (((cpu->af.b.h ^ _tmp ^ _value) & 0x10) << 1) | \
        0x40 | ((_tmp >> 4) & 0x10); \
    cpu->af.b.h = _tmp; \
}

#define GB_AND() {   \
    uint32 _tmp = cpu->af.b.h & _value; \
    cpu->af.b.l = ((!_tmp) << 7) | 0x10; \
    cpu->af.b.h = _tmp; \
}

#define GB_XOR() {   \
    uint32 _tmp = cpu->af.b.h ^ _value; \
    cpu->af.b.l = ((!_tmp) << 7); \
    cpu->af.b.h = _tmp; \
}

#define GB_OR() {   \
    uint32 _tmp = cpu->af.b.h | _value; \
    cpu->af.b.l = ((!_tmp) << 7); \
    cpu->af.b.h = _tmp; \
}

#define GB_CP() {   \
    uint32 _tmp = cpu->af.b.h - _value; \
    cpu->af.b.l = ((!((uint8)_tmp)) << 7) | \
        (((cpu->af.b.h ^ _tmp ^ _value) & 0x10) << 1) | \
        0x40 | ((_tmp >> 4) & 0x10); \
}

#define GB_ADDHL() {   \
    uint32 _tmp = cpu->hl.w + _value; \
    cpu->af.b.l = (cpu->af.b.l & 0x8F) | \
        (((cpu->hl.w ^ _tmp ^ _value) >> 7) & 0x20) | \
        ((_tmp >> 12) & 0x10); \
    cpu->hl.w = _tmp; \
}
