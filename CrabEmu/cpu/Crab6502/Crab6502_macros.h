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

#ifndef CRAB6502_MACROS_H
#define CRAB6502_MACROS_H

#ifndef CRAB6502_NO_READMAP_FALLBACK

#define FETCH_ARG8(name) \
    name = cpu->readmap[cpu->pc.w >> 8] ? \
        cpu->readmap[cpu->pc.w >> 8][(uint8)cpu->pc.w] : \
        cpu->mread(cpu, cpu->pc.w);  \
    ++cpu->pc.w;

#define FETCH_ARG16(name) { \
    uint16 pc2 = cpu->pc.w + 1; \
    uint8 pc_s = cpu->pc.w >> 8; \
    uint8 pc2_s = pc2 >> 8; \
    name = cpu->readmap[pc_s] ? \
        cpu->readmap[pc_s][(uint8)cpu->pc.w] : \
        cpu->mread(cpu, cpu->pc.w) | \
        ((cpu->readmap[pc2_s] ? \
          cpu->readmap[pc2_s][(uint8)pc2] : \
          cpu->mread(cpu, pc2)) << 8); \
    cpu->pc.w += 2; \
}

#ifdef CRAB6502_ZPG_USE_READMAP

#define FETCH_ADDR_ZPG(addr, name) { \
    uint8 __a = (uint8)(addr); \
    uint8 __a2 = __a + 1; \
    if(cpu->readmap[0]) \
        name = (cpu->readmap[0][__a]) | (cpu->readmap[0][__a2] << 8); \
    else \
        name = (cpu->mread(cpu, __a)) | (cpu->mread(cpu, __a2) << 8); \
}

#endif /* !CRAB6502_ZPG_USE_READMAP */

#ifdef CRAB6502_STACK_USE_READMAP

#define POP_STACK(name) { \
    uint16 __a = ++cpu->s | 0x0100; \
    uint16 _addr_s = __a >> 8; \
    name = cpu->readmap[_addr_s] ? \
        cpu->readmap[_addr_s][__a] : \
        cpu->mread(cpu, __a);  \
}

#endif /* CRAB6502_STACK_USE_READMAP */

#else /* CRAB6502_NO_READMAP_FALLBACK */

#define FETCH_ARG8(name) \
    name = cpu->readmap[cpu->pc.w >> 8][(uint8)cpu->pc.w]; \
    ++cpu->pc.w;

#define FETCH_ARG16(name) { \
    uint16 pc2 = cpu->pc.w + 1; \
    name = cpu->readmap[cpu->pc.w >> 8][(uint8)cpu->pc.w] | \
        (cpu->readmap[pc2 >> 8][(uint8)pc2] << 8); \
    cpu->pc.w += 2; \
}

#ifdef CRAB6502_ZPG_USE_READMAP

#define FETCH_ADDR_ZPG(addr, name) { \
    uint8 __a = (uint8)(addr); \
    uint8 __a2 = __a + 1; \
    name = (cpu->readmap[0][__a]) | (cpu->readmap[0][__a2] << 8); \
}

#endif /* CRAB6502_ZPG_USE_READMAP */

#ifdef CRAB6502_STACK_USE_READMAP

#define POP_STACK(name) { \
    uint16 __a = ++cpu->s | 0x0100; \
    name = cpu->readmap[__a >> 8][(uint8)__a]; \
}

#endif /* CRAB6502_STACK_USE_READMAP */

#endif /* CRAB6502_NO_READMAP_FALLBACK */

#define FETCH_BYTE(addr, name) { \
    uint16 __a = (addr); \
    name = cpu->mread(cpu, __a); \
}

#define FETCH_ADDR(addr, name) { \
    uint16 __a = (addr); \
    name = (cpu->mread(cpu, __a)) | (cpu->mread(cpu, __a + 1) << 8); \
}

#ifndef CRAB6502_ZPG_USE_READMAP

#define FETCH_ADDR_ZPG(addr, name) { \
    FETCH_ADDR(addr, name); \
}

#endif /* !CRAB6502_ZPG_USE_READMAP */

#ifndef CRAB6502_STACK_USE_READMAP

#define POP_STACK(name) { \
    uint16 __a = ++cpu->s | 0x0100; \
    name = cpu->mread(cpu, __a); \
}

#endif /* !CRAB6502_STACK_USE_READMAP */

#endif /* !CRAB6502_MACROS_H */
