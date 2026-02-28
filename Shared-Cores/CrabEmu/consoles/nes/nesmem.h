/*
    This file is part of CrabEmu.

    Copyright (C) 2012, 2013 Lawrence Sebald

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

#ifndef NESMEM_H
#define NESMEM_H

#include "CrabEmu.h"
#include <stddef.h>
#include <stdio.h>

CLINKAGE

/* Various stuff related to mappers... */
typedef struct nes_mapper {
    int mapper_num;

    void (*init)(void);
    void (*reset)(void);
    uint8 (*read)(void *cpu, uint16 addr);
    void (*write)(void *cpu, uint16 addr, uint8 val);
    int (*write_cxt)(FILE *fp);
    int (*read_cxt)(const uint8 *buf);
    uint32 (*cxt_len)(void);
} nes_mapper_t;

/* iNES header */
struct ines {
    uint8 sig[4];
    uint8 prg_size;
    uint8 chr_size;
    uint8 flags[2];
    uint8 prg_ram_size;
    uint8 flags2[2];
    uint8 reserved[5];
};

/* Various globals from nesmem.c */
extern uint8 nes_ram[2 * 1024];
extern uint8 *nes_read_map[256];
extern uint8 *nes_write_map[256];

extern uint32 nes_mapper;
extern size_t nes_prg_rom_size;
extern size_t nes_chr_rom_size;
extern size_t nes_sram_size;

extern uint8 *nes_prg_rom;
extern uint8 *nes_chr_rom;
extern uint8 *nes_sram;
extern struct ines nes_cur_hdr;

extern uint16 nes_pad;

extern int nes_mem_init(void);
extern int nes_mem_shutdown(void);
extern void nes_mem_reset(void);

extern void nes_get_checksums(uint32 *crc, uint32 *adler);

extern void nes_mem_update_map(void);

extern int nes_mem_write_sram(const char *fn);
extern int nes_mem_read_sram(const char *fn);

extern int nes_mem_load_rom(const char *fn);

extern uint8 nes_mem_read(void *cpu, uint16 addr);
extern uint8 nes_mem_readreg(uint16 addr);
extern void nes_mem_writereg(uint16 addr, uint8 val);

/* Save state stuff... */
extern int nes_game_write_context(FILE *fp);
extern int nes_game_read_context(const uint8 *buf);
extern int nes_mem_write_context(FILE *fp);
extern int nes_chr_ram_read_context(const uint8 *buf);
extern int nes_sram_read_context(const uint8 *buf);
extern int nes_mem_read_context(const uint8 *buf);
extern int nes_mapper_read_context(const uint8 *buf);

ENDCLINK

#endif /* !NESMEM_H */
