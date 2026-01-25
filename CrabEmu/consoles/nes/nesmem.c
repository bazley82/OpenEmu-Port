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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef _arch_dreamcast
#include <inttypes.h>
#include <kos/fs.h>
#include <dc/vmufs.h>
#include <dc/vmu_pkg.h>
#include <dc/maple.h>
#include <zlib/zlib.h>
#include <bzlib/bzlib.h>
#include "icon.h"
#else
#ifndef NO_ZLIB
#include <zlib.h>
#endif

#ifndef NO_BZ2
#include <bzlib.h>
#endif
#endif

#include "nesmem.h"
#include "nesppu.h"
#include "nesapu.h"
#include "Crab6502.h"
#include "rom.h"

#ifndef NO_ZLIB
#include "unzip.h"
#endif

uint8 nes_ram[2 * 1024];
uint8 *nes_read_map[256];
uint8 *nes_write_map[256];

static uint8 dummy_arear[256];
static uint8 dummy_areaw[256];

uint32 nes_mapper = 0;
size_t nes_prg_rom_size = 0;
size_t nes_chr_rom_size = 0;
size_t nes_sram_size;

uint8 *nes_prg_rom = NULL;
uint8 *nes_chr_rom = NULL;
uint8 *nes_sram = NULL;
struct ines nes_cur_hdr;
uint32 nes_prg_crc = 0, nes_prg_adler = 0;

uint16 nes_pad = 0;
static uint8 pad_latch[2] = { 0xFF, 0xFF };

static uint8 *rom_data = NULL;

extern Crab6502_t nescpu;

/* Various mappers */
extern nes_mapper_t nes_mapper_0, nes_mapper_1, nes_mapper_2, nes_mapper_3;
extern nes_mapper_t nes_mapper_7, nes_mapper_9, nes_mapper_66;

static nes_mapper_t *mappers[] = {
    &nes_mapper_0, &nes_mapper_1, &nes_mapper_2, &nes_mapper_3, &nes_mapper_7,
    &nes_mapper_9, &nes_mapper_66,
    NULL
};

static nes_mapper_t *cur_mapper = NULL;

uint8 nes_mem_read(void *cpu, uint16 addr) {
    return cur_mapper->read(cpu, addr);
}

uint8 nes_mem_readreg(uint16 addr) {
    uint8 rv;

    switch(addr) {
        case 0x4015:
            return nes_apu_read(addr);

        case 0x4016:
            rv = (pad_latch[0] & 0x01);
            pad_latch[0] = (pad_latch[0] >> 1) | 0x80;
            return rv;

        case 0x4017:
            rv = (pad_latch[1] & 0x01);
            pad_latch[1] = (pad_latch[1] >> 1) | 0x80;
            return rv;

        default:
            return 0xFF;
    }
}

void nes_mem_writereg(uint16 addr, uint8 val) {
    switch(addr) {
        case 0x4000:
        case 0x4001:
        case 0x4002:
        case 0x4003:
        case 0x4004:
        case 0x4005:
        case 0x4006:
        case 0x4007:
        case 0x4008:
        case 0x4009:
        case 0x400A:
        case 0x400B:
        case 0x400C:
        case 0x400D:
        case 0x400E:
        case 0x400F:
        case 0x4010:
        case 0x4011:
        case 0x4012:
        case 0x4013:
        case 0x4015:
        case 0x4017:
            nes_apu_write(addr, val);
            break;

        case 0x4014:
            nes_ppu_write_oamdma(val);
            nescpu.cycles_burned += 513;
            break;

        case 0x4016:
            if(val & 0x01) {
                pad_latch[0] = (uint8)nes_pad;
                pad_latch[1] = nes_pad >> 8;
            }
            break;
    }
}

static nes_mapper_t *find_mapper(int m) {
    int i;

    for(i = 0; mappers[i]; ++i) {
        if(mappers[i]->mapper_num == m)
            return mappers[i];
    }

    return NULL;
}

static void nes_finalize(const char *fn) {
    char *name;

    /* Calculate the CRC of the PRG */
    nes_prg_crc = rom_crc32(nes_prg_rom, nes_prg_rom_size);
    nes_prg_adler = rom_adler32(nes_prg_rom, nes_prg_rom_size);

    cur_mapper->init();
    Crab6502_set_memread(&nescpu, cur_mapper->read);
    Crab6502_set_memwrite(&nescpu, cur_mapper->write);
    Crab6502_set_readmap(&nescpu, nes_read_map);
    Crab6502_reset(&nescpu);

    if(fn) {
        name = strrchr(fn, '/');

        if(name != NULL) {
            /* we have a filename, but the first char is /, so skip that */
            gui_set_title(name + 1);
        }
        else {
            gui_set_title("CrabEmu");
        }
    }
}

void nes_mem_update_map(void) {
    Crab6502_set_readmap(&nescpu, nes_read_map);
}

static int nes_sram_written(void) {
    size_t i;

    for(i = 0; i < nes_sram_size; ++i) {
        if(nes_sram[i])
            return 1;
    }

    return 0;
}

#ifndef _arch_dreamcast
int nes_mem_write_sram(const char *fn) {
    FILE *fp;

    if(!fn)
        return -1;

    /* Make sure the SRAM has been written to */
    if(!nes_sram_written())
        return 0;

    if(!(fp = fopen(fn, "wb")))
        return -1;

    if(fwrite(nes_sram, 1, nes_sram_size, fp) != nes_sram_size) {
        fclose(fp);
        return -2;
    }

    fclose(fp);
    return 0;
}

int nes_mem_read_sram(const char *fn) {
    FILE *fp;

    if(!fn)
        return -1;

    if(!(fp = fopen(fn, "rb")))
        return -1;

    if(fread(nes_sram, 1, nes_sram_size, fp) != nes_sram_size) {
        fclose(fp);
        return -2;
    }

    fclose(fp);
    return 0;
}
#else
int nes_mem_write_sram(const char *fn) {
    vmu_pkg_t pkg;
    uint8 *pkg_out, *comp;
    int pkg_size, err;
    uint32 len;
    FILE *fp;
    char savename[32];
    maple_device_t *vmu;
    int rv = 0, blocks_freed = 0;
    file_t f;
    uLong max_sz;

    /* Make sure the SRAM has been written to */
    if(!nes_sram_written())
        return 0;

    /* Make sure there's a VMU in port A1. */
    if(!(vmu = maple_enum_dev(0, 1))) {
        return -100;
    }

    if(!vmu->valid || !(vmu->info.functions & MAPLE_FUNC_MEMCARD)) {
        return -100;
    }

    sprintf(savename, "/vmu/a1/nes%08" PRIX32, (uint32_t)nes_prg_crc);

    /* Figure out how much memory we need to compress and allocate it. */
    max_sz = compressBound(nes_sram_size);
    comp = (uint8 *)malloc(max_sz);
    len = max_sz;

    /* Compress the SRAM */
    err = compress2(comp, &len, nes_sram, nes_sram_size, 9);

    if(err != Z_OK) {
        free(comp);
        return -1;
    }

    if(fn != NULL) {
        strncpy(pkg.desc_long, fn, 32);
        pkg.desc_long[31] = 0;
    }
    else {
        sprintf(pkg.desc_long, "CRC: %08" PRIX32, (uint32_t)nes_prg_crc);
    }

    strcpy(pkg.desc_short, "CrabEmu Save");
    strcpy(pkg.app_id, "CrabEmu");
    pkg.icon_cnt = 1;
    pkg.icon_anim_speed = 0;
    memcpy(pkg.icon_pal, icon_pal, 32);
    pkg.icon_data = icon_img;
    pkg.eyecatch_type = VMUPKG_EC_NONE;
    pkg.data_len = len;
    pkg.data = comp;

    vmu_pkg_build(&pkg, &pkg_out, &pkg_size);

    /* See if a file exists with that name, since we'll overwrite it. */
    f = fs_open(savename, O_RDONLY);
    if(f != FILEHND_INVALID) {
        blocks_freed = fs_total(f) >> 9;
        fs_close(f);
    }

    /* Make sure there's enough free space on the VMU. */
    if(vmufs_free_blocks(vmu) + blocks_freed < (pkg_size >> 9)) {
        free(pkg_out);
        free(comp);
        return pkg_size >> 9;
    }

    if(!(fp = fopen(savename, "wb"))) {
        free(pkg_out);
        free(comp);
        return -1;
    }

    if(fwrite(pkg_out, 1, pkg_size, fp) != (size_t)pkg_size)
        rv = -1;

    fclose(fp);

    free(pkg_out);
    free(comp);

    return rv;
}

int nes_mem_read_sram(const char *fn __UNUSED__) {
    vmu_pkg_t pkg;
    uint8 *pkg_out;
    int pkg_size;
    FILE *fp;
    char savename[32];
    uint32 real_size = nes_sram_size;

    /* Try to open the file */
    sprintf(savename, "/vmu/a1/nes%08" PRIX32, (uint32_t)nes_prg_crc);

    if(!(fp = fopen(savename, "rb")))
        return -1;

    fseek(fp, 0, SEEK_SET);
    fseek(fp, 0, SEEK_END);
    pkg_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    pkg_out = (uint8 *)malloc(pkg_size);
    fread(pkg_out, pkg_size, 1, fp);
    fclose(fp);

    vmu_pkg_parse(pkg_out, &pkg);

    uncompress(nes_sram, &real_size, pkg.data, pkg.data_len);
    free(pkg_out);

    return 0;
}
#endif /* _arch_dreamcast */

static int parse_rom(const char *fn, int size) {
    struct ines hdr;

    /* Read in the iNES header */
    memcpy(&hdr, rom_data, sizeof(struct ines));

    if(hdr.sig[0] != 'N' || hdr.sig[1] != 'E' || hdr.sig[2] != 'S' ||
       hdr.sig[3] != 0x1A) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: ROM %s not an iNES rom!\n", fn);
#endif
        free(rom_data);
        rom_data = NULL;

        return ROM_LOAD_E_NO_ROM;
    }

    nes_prg_rom_size = hdr.prg_size * 16384;
    nes_chr_rom_size = hdr.chr_size * 8192;

    /* Check if the header is clean or if we need to ignore the high nibble of
       the mapper. */
    if(hdr.reserved[4] != 0 || hdr.reserved[3] != 0 || hdr.reserved[2] != 0 ||
       hdr.reserved[1] != 0) {
        nes_mapper = hdr.flags[0] >> 4;
        nes_sram_size = 8192;
    }
    else {
        nes_mapper = (hdr.flags[0] >> 4) | (hdr.flags[1] & 0xF0);
        nes_sram_size = hdr.prg_ram_size ? hdr.prg_ram_size * 8192 : 8192;
    }

#ifdef DEBUG
    printf("PRG ROM Size: %d, CHR ROM Size: %d, SRAM Size: %d\n",
           (int)nes_prg_rom_size, (int)nes_chr_rom_size, (int)nes_sram_size);
    printf("iNES Mapper: %d (CTL: %02x %02x)\n", nes_mapper, hdr.flags[0],
           hdr.flags[1]);
#endif

    /* Make sure we support the mapper. */
    if(!(cur_mapper = find_mapper(nes_mapper))) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Mapper %d is unsupported!\n",
                nes_mapper);
#endif
        free(rom_data);
        rom_data = NULL;

        return ROM_LOAD_E_UNK_MAPPER;
    }

    /* Make sure there's no trainer */
    if(hdr.flags[0] & 0x04) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: ROM has an unsupported trainer!\n");
#endif
        free(rom_data);
        rom_data = NULL;

        return ROM_LOAD_E_HAS_TRAINER;
    }

    /* Make sure the file is the right size */
    if(size < (int)(nes_prg_rom_size + nes_chr_rom_size + 16)) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: ROM is too small (got %d, expected "
                "%d)\n", size, (int)(nes_prg_rom_size + nes_chr_rom_size + 16));
#endif
        free(rom_data);
        rom_data = NULL;

        return ROM_LOAD_E_BAD_SZ;
    }

    /* Set up the pointers for PRG and CHR ROM */
    nes_prg_rom = rom_data + sizeof(struct ines);
    nes_chr_rom = nes_prg_rom + nes_prg_rom_size;

    /* Make space for SRAM and CHR RAM (if needed). */
    if(!(nes_sram = (uint8 *)malloc(nes_sram_size))) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Couldn't allocate SRAM\n");
        perror("malloc");
#endif
        free(rom_data);
        rom_data = nes_prg_rom = nes_chr_rom = NULL;

        return ROM_LOAD_E_ERRNO;
    }

    /* Clear the SRAM so we can detect if its written later... */
    memset(nes_sram, 0, nes_sram_size);

    if(!nes_chr_rom_size) {
        if(!(nes_chr_rom = (uint8 *)malloc(8192))) {
#ifdef DEBUG
            fprintf(stderr, "nes_mem_load_rom: Couldn't allocate CHR RAM\n");
            perror("malloc");
#endif
            free(rom_data);
            free(nes_sram);
            rom_data = nes_prg_rom = nes_sram = NULL;

            return ROM_LOAD_E_ERRNO;
        }
    }

    nes_cur_hdr = hdr;

    nes_finalize(fn);
    return ROM_LOAD_SUCCESS;
}

#ifndef NO_ZLIB
static int load_gz_rom(const char *fn) {
    int len, size = 0;
    gzFile fp;
    void *tmp;

    /* Open up the file in question. */
    if(!(fp = gzopen(fn, "rb"))) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_OPEN;
    }

    if(!(rom_data = (uint8 *)malloc(0x1000))) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Couldn't allocate space!\n");
        perror("malloc");
#endif
        gzclose(fp);
        return ROM_LOAD_E_ERRNO;
    }

    size = 0;
    len = gzread(fp, rom_data, 0x1000);

    /* Read in a block of the file at a time... */
    while(len != 0 && len != -1) {
        size += len;
        tmp = realloc(rom_data, size + 0x1000);

        if(!tmp) {
#ifdef DEBUG
            fprintf(stderr, "nes_mem_load_rom: Couldn't allocate space!\n");
            perror("realloc");
#endif
            gzclose(fp);
            free(rom_data);
            rom_data = NULL;

            return ROM_LOAD_E_ERRNO;
        }

        rom_data = (uint8 *)tmp;
        memset(rom_data + size, 0xFF, 0x1000);
        len = gzread(fp, rom_data + size, 0x1000);
    }

    /* Truncate to the "correct" length if we can. */
    if((tmp = realloc(rom_data, size)))
        rom_data = (uint8 *)tmp;

    /* Close the file, as we're done with it */
    gzclose(fp);

    /* Parse the file we read in... */
    return parse_rom(fn, size);
}
#endif

#ifndef NO_BZ2
static int load_bz2_rom(const char *fn) {
    int len, size = 0;
    BZFILE *fp;
    void *tmp;

    /* Open up the file in question. */
    if(!(fp = BZ2_bzopen(fn, "rb"))) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_OPEN;
    }

    if(!(rom_data = (uint8 *)malloc(0x1000))) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Couldn't allocate space!\n");
        perror("malloc");
#endif
        BZ2_bzclose(fp);
        return ROM_LOAD_E_ERRNO;
    }

    size = 0;
    len = BZ2_bzread(fp, rom_data, 0x1000);

    /* Read in a block of the file at a time... */
    while(len != 0 && len != -1) {
        size += len;
        tmp = realloc(rom_data, size + 0x1000);

        if(!tmp) {
#ifdef DEBUG
            fprintf(stderr, "nes_mem_load_rom: Couldn't allocate space!\n");
            perror("realloc");
#endif
            BZ2_bzclose(fp);
            free(rom_data);
            rom_data = NULL;

            return ROM_LOAD_E_ERRNO;
        }

        rom_data = (uint8 *)tmp;
        memset(rom_data + size, 0xFF, 0x1000);
        len = BZ2_bzread(fp, rom_data + size, 0x1000);
    }

    /* Truncate to the "correct" length if we can. */
    if((tmp = realloc(rom_data, size)))
        rom_data = (uint8 *)tmp;

    /* Close the file, as we're done with it */
    BZ2_bzclose(fp);

    /* Parse the file we read in... */
    return parse_rom(fn, size);
}
#endif

#ifndef NO_ZLIB
static int load_zip_rom(const char *fn) {
    unzFile fp;
    unz_file_info inf;
    char fn2[4096];
    char *ext;
    int found = 0, size;

    /* Open up the zip file in question */
    if(!(fp = unzOpen(fn))) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_OPEN;
    }

    if(unzGoToFirstFile(fp) != UNZ_OK) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Couldn't find file in zip!\n");
#endif
        unzClose(fp);
        return ROM_LOAD_E_CORRUPT;
    }

    /* Keep looking at files until we find a rom. */
    while(!found) {
        if(unzGetCurrentFileInfo(fp, &inf, fn2, 4096, NULL, 0, NULL,
                                 0) != UNZ_OK) {
#ifdef DEBUG
            fprintf(stderr, "nes_mem_load_rom: Error parsing zip file!\n");
#endif
            unzClose(fp);
            return ROM_LOAD_E_CORRUPT;
        }

        /* Check if this file looks like a rom. */
        ext = strrchr(fn2, '.');

        if(ext) {
            if(!strcmp(ext, ".nes"))
                found = 1;
        }

        /* If we haven't found a rom yet, move to the next file. */
        if(!found) {
            if(unzGoToNextFile(fp) != UNZ_OK) {
#ifdef DEBUG
                fprintf(stderr, "nes_mem_load_rom: End of zip file!\n");
#endif
                unzClose(fp);
                return ROM_LOAD_E_NO_ROM;
            }
        }
    }

    /* Allocate space for the file, and read it in */
    if(!(rom_data = (uint8 *)malloc(inf.uncompressed_size))) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Couldn't allocate space for ROM!\n");
        perror("malloc");
#endif
        unzClose(fp);
        return ROM_LOAD_E_ERRNO;
    }

    size = inf.uncompressed_size;

    /* Attempt to open the file we think is a rom. */
    if(unzOpenCurrentFile(fp) != UNZ_OK) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Couldn't open rom from zip!\n");
#endif
        unzClose(fp);
        free(rom_data);
        rom_data = NULL;

        return ROM_LOAD_E_CORRUPT;
    }

    /* Attempt to read the file. */
    if(unzReadCurrentFile(fp, rom_data, size) < 0) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Couldn't read rom from zip!\n");
#endif
        unzCloseCurrentFile(fp);
        unzClose(fp);
        free(rom_data);
        rom_data = NULL;
        return ROM_LOAD_E_CORRUPT;
    }

    /* We're done with the file, close everything. */
    unzCloseCurrentFile(fp);
    unzClose(fp);

    /* Parse the file we read in... */
    return parse_rom(fn, size);
}
#endif

int nes_mem_load_rom(const char *fn) {
    char *ext;
    FILE *fp;
    int size;

    /* If its a compressed format, go onto their handler... */
    ext = strrchr(fn, '.');

    if(ext) {
#ifndef NO_ZLIB
        if(!strcmp(ext, ".gz")) {
            return load_gz_rom(fn);
        }
        else if(!strcmp(ext, ".zip")) {
            return load_zip_rom(fn);
        }
#endif
#ifndef NO_BZ2
        if(!strcmp(ext, ".bz2")) {
            return load_bz2_rom(fn);
        }
#endif
    }

    if(!(fp = fopen(fn, "rb"))) {
#ifdef DEBUG
        perror("fopen");
        fprintf(stderr, "nes_mem_load_rom: Could not open ROM: %s!\n", fn);
#endif
        return ROM_LOAD_E_ERRNO;
    }

    /* Determine the size of the file */
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if(size < 16) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: ROM %s too short!\n", fn);
#endif
        fclose(fp);
        return ROM_LOAD_E_BAD_SZ;
    }

    /* Allocate space for the ROM's data. */
    if(!(rom_data = (uint8 *)malloc(size))) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Couldn't allocate space for ROM!\n");
        perror("malloc");
#endif
        fclose(fp);
        return ROM_LOAD_E_ERRNO;
    }

    /* Read the file in */
    if(fread(rom_data, 1, size, fp) != (size_t)size) {
#ifdef DEBUG
        fprintf(stderr, "nes_mem_load_rom: Couldn't read ROM file!\n");
        perror("fread");
#endif
        free(rom_data);
        rom_data = NULL;
        fclose(fp);

        return ROM_LOAD_E_ERRNO;
    }

    /* Close the file, as we're done with it */
    fclose(fp);

    /* Parse the file we read in... */
    return parse_rom(fn, size);
}

int nes_mem_init(void) {
    int i;

    for(i = 0x20; i < 0x100; ++i) {
        nes_read_map[i] = dummy_arear;
        nes_write_map[i] = dummy_areaw;
    }

    for(i = 0; i < 0x20; ++i) {
        nes_read_map[i] = nes_ram + ((i & 0x07) << 8);
        nes_write_map[i] = nes_ram + ((i & 0x07) << 8);
    }

    Crab6502_set_readmap(&nescpu, nes_read_map);

    memset(nes_ram, 0xFF, 2 * 1024);
    memset(dummy_arear, 0xFF, 256);

    nes_pad = 0;
    pad_latch[0] = pad_latch[1] = 0xFF;

    return 0;
}

int nes_mem_shutdown(void) {
    if(!nes_chr_rom_size)
        free(nes_chr_rom);

    free(nes_sram);
    free(rom_data);

    nes_prg_rom_size = nes_chr_rom_size = nes_sram_size = 0;
    nes_prg_rom = nes_chr_rom = nes_sram = rom_data = NULL;
    nes_mapper = 0;
    cur_mapper = NULL;

    memset(&nes_cur_hdr, 0, sizeof(nes_cur_hdr));

    return 0;
}

void nes_mem_reset(void) {
    int i;

    for(i = 0; i < 0x20; ++i) {
        nes_read_map[i] = nes_ram + ((i & 0x07) << 8);
        nes_write_map[i] = nes_ram + ((i & 0x07) << 8);
    }

    memset(nes_ram, 0xFF, 2 * 1024);
    memset(dummy_arear, 0xFF, 256);

    nes_pad = 0;
    pad_latch[0] = pad_latch[1] = 0xFF;

    cur_mapper->reset();
    Crab6502_set_readmap(&nescpu, nes_read_map);
}

void nes_get_checksums(uint32 *crc, uint32 *adler) {
    *crc = nes_prg_crc;
    *adler = nes_prg_adler;
}

int nes_game_write_context(FILE *fp) {
    uint8 data[4];

    data[0] = 'G';
    data[1] = 'A';
    data[2] = 'M';
    data[3] = 'E';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(24, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    UINT16_TO_BUF(0, data);
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 0) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    UINT32_TO_BUF(nes_prg_crc, data);
    fwrite(data, 1, 4, fp);

    UINT32_TO_BUF(nes_prg_adler, data);
    fwrite(data, 1, 4, fp);

    return 0;
}

int nes_game_read_context(const uint8 *buf) {
    uint32 len, crc;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 24)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Check if the game matches... */
    BUF_TO_UINT32(buf + 16, crc);
    if(crc != nes_prg_crc)
        return -2;

    BUF_TO_UINT32(buf + 20, crc);
    if(crc != nes_prg_adler)
        return -2;

    return 0;
}

static int nes_chr_ram_write_context(FILE *fp) {
    uint8 data[4];

    /* Make sure we have CHR RAM (not ROM) */
    if(nes_chr_rom_size)
        return 0;

    data[0] = 'N';
    data[1] = 'C';
    data[2] = 'R';
    data[3] = 'M';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(8208, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(nes_chr_rom, 1, 8192, fp);

    return 0;
}

int nes_chr_ram_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Make sure we have CHR RAM (not ROM) */
    if(nes_chr_rom_size)
        return 0;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 8208)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Copy in the data */
    memcpy(nes_chr_rom, buf + 16, 8192);

    return 0;
}

static int nes_sram_write_context(FILE *fp) {
    uint8 data[4];
    uint32 len = nes_sram_size;

    /* Make sure we have SRAM */
    if(!nes_sram_size)
        return 0;

    data[0] = 'N';
    data[1] = 'S';
    data[2] = 'R';
    data[3] = 'M';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(len + 16, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(nes_sram, 1, len, fp);

    return 0;
}

int nes_sram_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Make sure we have SRAM */
    if(!nes_sram_size)
        return 0;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != nes_sram_size + 16)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Copy in the data */
    memcpy(nes_sram, buf + 16, nes_sram_size);

    return 0;
}

int nes_mem_write_context(FILE *fp) {
    uint8 data[4];
    int cxt_len;

    data[0] = 'D';
    data[1] = 'R';
    data[2] = 'A';
    data[3] = 'M';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(2064, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(nes_ram, 1, 2048, fp);

    /* Write the mapper block */
    data[0] = 'M';
    data[1] = 'A';
    data[2] = 'P';
    data[3] = 'R';
    fwrite(data, 1, 4, fp);             /* Block ID */

    cxt_len = cur_mapper->cxt_len();
    UINT32_TO_BUF(20 + cxt_len, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    /* Child pointer */
    if(cxt_len) {
        UINT32_TO_BUF(4, data);
    }
    else {
        UINT32_TO_BUF(0, data);
    }

    fwrite(data, 1, 4, fp);

    UINT32_TO_BUF(nes_mapper, data);
    fwrite(data, 1, 4, fp);
    
    if(cxt_len) {
        if(cur_mapper->write_cxt(fp))
            return -1;
    }

    /* If we have CHR RAM, write it */
    if(!nes_chr_rom_size) {
        if(nes_chr_ram_write_context(fp))
            return -1;
    }

    /* If we have SRAM, write it */
    if(nes_sram_size) {
        if(nes_sram_write_context(fp))
            return -1;
    }

    return 0;
}

int nes_mem_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 2064)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Copy in the data */
    memcpy(nes_ram, buf + 16, 2048);

    return 0;
}

int nes_mapper_read_context(const uint8 *buf) {
    uint32 len, ptr, num;
    uint16 ver;
    uint32 cxt_len = cur_mapper->cxt_len();

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != cxt_len + 20)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(!cxt_len) {
        ptr = 0;
        if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
            return -1;
    }
    else {
        BUF_TO_UINT32(buf + 12, ptr);
        if(ptr != 4)
            return -1;
    }

    /* Read in the mapper number */
    BUF_TO_UINT32(buf + 16, num);
    if(num != nes_mapper)
        return -1;

    /* Read in the mapper data, if any */
    if(ptr)
        return cur_mapper->read_cxt(buf + ptr + 16);

    return 0;
}
