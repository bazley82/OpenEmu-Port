/*
    This file is part of CrabEmu.

    Copyright (C) 2012, 2013, 2014 Lawrence Sebald

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
#include <string.h>

#ifdef _arch_dreamcast
#include <malloc.h>
#endif

#include "nesppu.h"
#include "nesmem.h"

static uint16 ppu_lut[256];

static uint8 ppu_regs[8];
static int ppu_addr_latched = 0;
static uint16 ppu_v = 0;
static uint16 ppu_t;
static uint8 ppu_x;
static uint8 ppu_buffer = 0;

static uint8 ppu_oam_addr = 0;
static uint8 ppu_oam_ram[256];

static uint8 ppu_bg_pal[16], real_bg_pal[16];
static uint8 ppu_spr_pal[16], real_spr_pal[16];

static uint8 *ppu_map[0x30];
static uint8 ppu_nametables[4096];

static int ppu_patterns_rom = 0;
static nes_ppu_pattern_t ppu_patterns[512];

static pixel_t *ppu_framebuffer;
static void (*mmc2_cb)(uint16 pn);

struct ppu_spr {
    int pattern;
    uint8 spr_num;
    uint8 tex;
    uint8 x;
    uint8 y;
    uint8 pal;
    uint8 bg;
};

static int sprs_per_line[240];
static struct ppu_spr sprs[240][8];

void nes_ppu_update_cache(int pat) {
    uint8 *bitplane;
    int i, j;
    uint8 pixel;
    uint8 *tex1, *tex2, *tex3, *tex4;
    int tmp[16];
    uint16 lutval;

    if(pat >= 512 || !ppu_patterns[pat].dirty)
        return;

    /* Determine where we should start converting */
    bitplane = ppu_map[pat >> 4] + ((pat & 0x0F) << 4);

    for(i = 0; i < 16; ++i) {
        tmp[i] = *bitplane++;
    }

    tex1 = ppu_patterns[pat].texture[0] + 7;
    tex2 = ppu_patterns[pat].texture[1];
    tex3 = ppu_patterns[pat].texture[2] + 63;
    tex4 = ppu_patterns[pat].texture[3] + 56;

    for(i = 0; i < 8; ++i) {
        /* lutval = 8x2bpp pixels */
        lutval = (ppu_lut[tmp[i]]) | (ppu_lut[tmp[i + 8]] << 1);

        for(j = 0; j < 8; ++j) {
            pixel = lutval & 0x03;

            *tex1-- = pixel;
            *tex2++ = pixel;
            *tex3-- = pixel;
            *tex4++ = pixel;

            /* Shift off the pixel we just looked at */
            lutval >>= 2;
        }

        tex1 += 16;
        tex4 -= 16;
    }

    ppu_patterns[pat].dirty = 0;
}

const nes_ppu_pattern_t *nes_ppu_fetch_pattern(int pat) {
    if(pat >= 512)
        return NULL;

    return ppu_patterns + pat;
}

void nes_ppu_fetch_bg_pal(uint8 pal[16]) {
    int i;

    if(ppu_regs[1] & 0x01) {
        for(i = 0; i < 16; ++i) {
            pal[i] = real_bg_pal[i] & 0x30;
        }
    }
    else {
        memcpy(pal, real_bg_pal, 16);
    }
}

void nes_ppu_fetch_spr_pal(uint8 pal[16]) {
    int i;

    if(ppu_regs[1] & 0x01) {
        for(i = 0; i < 16; ++i) {
            pal[i] = real_spr_pal[i] & 0x30;
        }
    }
    else {
        memcpy(pal, real_spr_pal, 16);
    }
}

void *nes_ppu_framebuffer(void) {
    return ppu_framebuffer;
}

void nes_ppu_framesize(uint32_t *x, uint32_t *y) {
    *x = 256;
    *y = 256;
}

void nes_ppu_activeframe(uint32_t *x, uint32_t *y, uint32_t *w, uint32_t *h) {
    *x = *y = 0;
    *w = 256;
    *h = 240;
}

void nes_ppu_writereg(int reg, uint8 val) {
    uint16 addr;

    reg &= 0x07;
    ppu_regs[reg] = val;

    /* The logic on t, v, and x here comes directly out of Loopy's "the skinny
       on nes scrolling". */
    switch(reg) {
        case 0:
            ppu_t = (ppu_t & 0xF3FF) | ((val & 0x03) << 10);
            break;

        case 3:
            ppu_oam_addr = val;
            break;

        case 4:
            if((ppu_oam_addr & 0x03) == 0x02)
                ppu_oam_ram[ppu_oam_addr++] = val & 0xE3;
            else
                ppu_oam_ram[ppu_oam_addr++] = val;
            break;

        case 5:
            if(!ppu_addr_latched) {
                ppu_t = (ppu_t & 0xFFE0) | (val >> 3);
                ppu_x = val & 0x07;
                ppu_addr_latched = 1;
            }
            else {
                ppu_t = (ppu_t & 0x8C1F) | ((val & 0xF8) << 2) |
                    ((val & 0x07) << 12);
                ppu_addr_latched = 0;
            }
            break;

        case 6:
            if(!ppu_addr_latched) {
                ppu_t = (ppu_t & 0x00FF) | ((val & 0x3F) << 8);
                ppu_addr_latched = 1;
            }
            else {
                ppu_t = (ppu_t & 0xFF00) | val;
                ppu_v = ppu_t;
                ppu_addr_latched = 0;
            }
            break;

        case 7:
            /* Grab the address and update what we're looking at for next time
               we try to read/write. */
            addr = ppu_v & 0x3FFF;
            ppu_v += (ppu_regs[0] & 0x04) ? 32 : 1;

            /* See where we're writing... */
            if(addr >= 0x3F00) {
                val &= 0x3F;

                /* Palette memory */
                if(addr & 0x0010) {
                    addr &= 0x000F;
                    ppu_spr_pal[addr] = val;
                    if(addr & 0x0003) {
                        real_spr_pal[addr] = val;
                    }
                    else {
                        ppu_bg_pal[addr] = val;
                        if(addr == 0)
                            real_bg_pal[0] = real_bg_pal[4] = real_bg_pal[8] =
                                real_bg_pal[12] = real_spr_pal[0] =
                                real_spr_pal[4] = real_spr_pal[8] =
                                real_spr_pal[12] = val;
                    }
                }
                else {
                    addr &= 0x000F;
                    ppu_bg_pal[addr] = val;
                    if(addr & 0x0003)
                        real_bg_pal[addr] = val;
                    else if(addr == 0)
                        real_bg_pal[0] = real_bg_pal[4] = real_bg_pal[8] =
                            real_bg_pal[12] = real_spr_pal[0] =
                            real_spr_pal[4] = real_spr_pal[8] =
                            real_spr_pal[12] = val;
                }

                break;
            }
            /* Make sure we handle any sort of mirroring caused by accessing
               0x3000-0x3EFF. */
            else if(addr >= 0x3000) {
                addr &= 0x2FFF;
            }

            if(addr >= 0x2000) {
                ppu_map[addr >> 8][(uint8)addr] = val;
            }
            else if(addr < 0x2000 && !ppu_patterns_rom) {
                ppu_map[addr >> 8][(uint8)addr] = val;
                ppu_patterns[addr >> 4].dirty = 1;
            }
            break;
    }
}

uint8 nes_ppu_readreg(int reg) {
    uint8 rv = ppu_regs[reg & 0x07];
    uint16 addr, addr2;

    switch(reg & 0x07) {
        case 2:
            /* Clear the VBlank flag and the address latch */
            ppu_regs[2] &= 0x7F;
            ppu_addr_latched = 0;
            break;

        case 7:
            /* Grab the address and update what we're looking at for next time
               we try to read/write. */
            addr = addr2 = ppu_v & 0x3FFF;
            ppu_v += (ppu_regs[0] & 0x04) ? 32 : 1;

            /* See where we're reading... */
            if(addr >= 0x3F00) {
                /* Palette memory */
                if(addr & 0x0010)
                    rv = ppu_spr_pal[addr & 0x000F];
                else
                    rv = ppu_bg_pal[addr & 0x000F];

                /* If we're in monochrome mode, mask out the lower bits. */
                if(ppu_regs[1] & 0x01)
                    rv &= 0x30;

                /* Mask the address and fill the buffer. */
                addr &= 0x2FFF;
                ppu_buffer = ppu_map[(addr >> 8)][(uint8)addr];
            }
            else {
                /* Make sure we handle any sort of mirroring caused by accessing
                   0x3000-0x3EFF. */
                if(addr >= 0x3000)
                    addr2 &= 0x2FFF;

                rv = ppu_buffer;
                ppu_buffer = ppu_map[(addr2 >> 8)][(uint8)addr2];

                /* Grumble... grumble... Stupid MMC2... */
                if(mmc2_cb) {
                    if((addr >= 0x0FD0 && addr < 0x0FF0) ||
                       (addr >= 0x1FD0 && addr < 0x1FF0))
                        mmc2_cb(addr >> 4);
                }
            }
            break;
    }

    return rv;
}

void nes_ppu_write_oamdma(uint8 val) {
    uint16 addr = val << 8;
    int i;

    for(i = 0; i < 64; ++i) {
        ppu_oam_ram[(uint8)((i << 2) + ppu_oam_addr)] =
            nes_mem_read(NULL, addr++);
        ppu_oam_ram[(uint8)((i << 2) + ppu_oam_addr + 1)] =
            nes_mem_read(NULL, addr++);
        ppu_oam_ram[(uint8)((i << 2) + ppu_oam_addr + 2)] =
            nes_mem_read(NULL, addr++) & 0xE3;
        ppu_oam_ram[(uint8)((i << 2) + ppu_oam_addr + 3)] =
            nes_mem_read(NULL, addr++);
    }
}

void nes_ppu_vblank_in(void) {
    ppu_regs[2] |= 0x80;
}

void nes_ppu_vblank_out(void) {
    ppu_regs[2] &= 0x3F;
}

int nes_ppu_vblnmi_enabled(void) {
    return ppu_regs[0] & 0x80;
}

void nes_ppu_set_tblmirrors(int n1, int n2, int n3, int n4) {
    ppu_map[0x20] = ppu_nametables + (n1 << 10);
    ppu_map[0x21] = ppu_nametables + (n1 << 10) + 0x100;
    ppu_map[0x22] = ppu_nametables + (n1 << 10) + 0x200;
    ppu_map[0x23] = ppu_nametables + (n1 << 10) + 0x300;
    ppu_map[0x24] = ppu_nametables + (n2 << 10);
    ppu_map[0x25] = ppu_nametables + (n2 << 10) + 0x100;
    ppu_map[0x26] = ppu_nametables + (n2 << 10) + 0x200;
    ppu_map[0x27] = ppu_nametables + (n2 << 10) + 0x300;
    ppu_map[0x28] = ppu_nametables + (n3 << 10);
    ppu_map[0x29] = ppu_nametables + (n3 << 10) + 0x100;
    ppu_map[0x2A] = ppu_nametables + (n3 << 10) + 0x200;
    ppu_map[0x2B] = ppu_nametables + (n3 << 10) + 0x300;
    ppu_map[0x2C] = ppu_nametables + (n4 << 10);
    ppu_map[0x2D] = ppu_nametables + (n4 << 10) + 0x100;
    ppu_map[0x2E] = ppu_nametables + (n4 << 10) + 0x200;
    ppu_map[0x2F] = ppu_nametables + (n4 << 10) + 0x300;
}

void nes_ppu_set_patterntbl(int tbl, uint8 *ptr, int rom) {
    int i;

    for(i = 0; i < 16; ++i) {
        ppu_map[(tbl << 4) + i] = ptr + (i << 8);
    }

    ppu_patterns_rom = rom;
    tbl <<= 8;

    /* Mark all the patterns in that table as dirty */
    for(i = 0; i < 256; ++i) {
        ppu_patterns[i + tbl].dirty = 1;
    }
}

/* Precalculate a whole frame's worth of sprite data. */
static void ppu_calc_spr(void) {
    int i, j, k, height, pth;
    int y, idx, attr, x;
    int pal, tex, pn, pn2, bg;

    /* Clear the tables... */
    memset(sprs_per_line, 0, sizeof(int) * 240);
    memset(sprs, 0xFF, sizeof(struct ppu_spr) * 8 * 240);

    /* If bit 5 of register 0 is set, sprites are 16 pixels high. The pattern
       table base is defined by the OAM data itself. */
    if(ppu_regs[0] & 0x20) {
        height = 16;
        pth = 0;
    }
    /* Otherwise, they're 8 pixels high, and the pattern table base is
       determined by bit 3 of register 0. */
    else {
        height = 8;
        pth = (ppu_regs[0] & 0x08) << 5;
    }

    for(i = 0; i < 64; ++i) {
        y = ppu_oam_ram[i << 2] + 1;

        if(y >= 240)
            continue;

        idx = ppu_oam_ram[(i << 2) + 1];
        attr = ppu_oam_ram[(i << 2) + 2];
        x = ppu_oam_ram[(i << 2) + 3];

        /* Figure out what we need to know from the attribute byte */
        pal = (attr & 0x03) << 2;
        tex = 0;
        bg = 0;

        if(attr & 0x80)
            tex = 2;
        if(attr & 0x40)
            ++tex;

        if(attr & 0x20)
            bg = 1;
        
        /* Figure out what pattern we're looking at */
        if(height == 8) {
            pn = idx + pth;
            pn2 = pn;
        }
        else {
            if(!(attr & 0x80)) {
                pn = (idx & 0xFE) | ((idx & 0x01) << 8);
                pn2 = pn + 1;
            }
            else {
                pn2 = (idx & 0xFE) | ((idx & 0x01) << 8);
                pn = pn2 + 1;
            }
        }

        for(j = 0; j < 8 && j + y < 240; ++j) {
            if((k = sprs_per_line[j + y]) < 8) {
                sprs[j + y][k].spr_num = i;
                sprs[j + y][k].pattern = pn;
                sprs[j + y][k].tex = tex;
                sprs[j + y][k].y = j;
                sprs[j + y][k].x = x;
                sprs[j + y][k].pal = pal;
                sprs[j + y][k].bg = bg;
            }

            ++sprs_per_line[j + y];
        }

        if(height == 16) {
            for(j = 8; j < 16 && j + y < 240; ++j) {
                if((k = sprs_per_line[j + y]) < 8) {
                    sprs[j + y][k].spr_num = i;
                    sprs[j + y][k].pattern = pn2;
                    sprs[j + y][k].tex = tex;
                    sprs[j + y][k].y = j;
                    sprs[j + y][k].x = x;
                    sprs[j + y][k].pal = pal;
                    sprs[j + y][k].bg = bg;
                }

                ++sprs_per_line[j + y];
            }
        }
    }
}

#define RENDER_BG_PIXEL() { \
    pixel = *pixels++; \
    *bgp++ = pixel; \
    *px++ = pixel + pal; \
}

static void ppu_bg_draw(int line __UNUSED__, uint8 *px,
                        uint8 bg_pixels[256 + 16]) {
    int xc, yc, yf, nt, at;
    int col, pal, pn, pth;
    uint8 ntb, atb, pixel;
    nes_ppu_pattern_t *pat;
    uint8 *bgp = bg_pixels, *pixels;
    uint8 *sp = px;

    /* Which half of the pattern table are we using? */
    pth = (ppu_regs[0] & 0x10) << 4;

    /* Grab the X and Y scroll values from the "V register". */
    xc = ppu_v & 0x1F;
    yc = (ppu_v >> 5) & 0x1F;
    yf = (ppu_v >> 12) & 0x07;

    /* Grab the name and attribute tables' addresses */
    nt = 0x2000 | (ppu_v & 0x0FFF);
    at = 0x23C0 + ((yc & 0x1C) << 1) + (xc >> 2) + (nt & 0x0C00);

    /* Grab the first attribute byte and set up the pointers... */
    atb = ppu_map[at >> 8][(uint8)at];
    px += 8 - ppu_x;
    bgp += 8 - ppu_x;

    for(col = 0; col < 33; ++col) {
        /* Grab the name table byte for this tile and the pattern that it
           corresponds to. */
        ntb = ppu_map[nt >> 8][(uint8)nt];
        pn = ntb + pth;
        pat = ppu_patterns + pn;

        /* Update the cache if the pattern is marked as dirty */
        if(pat->dirty)
            nes_ppu_update_cache(pn);

        /* Grab the palette base */
        pal = ((yc & 0x02) << 1) | (xc & 0x02);
        pal = ((atb >> pal) & 0x03) << 2;

        /* Render the pixels! */
        pixels = pat->texture[0] + (yf << 3);
        RENDER_BG_PIXEL();
        RENDER_BG_PIXEL();
        RENDER_BG_PIXEL();
        RENDER_BG_PIXEL();
        RENDER_BG_PIXEL();
        RENDER_BG_PIXEL();
        RENDER_BG_PIXEL();
        RENDER_BG_PIXEL();

        /* If we've got an MMC2 or an MMC4 and we need to set the latches, do
           so now. */
        if(mmc2_cb &&
           (pn == 0x0FD || pn == 0x0FE || pn == 0x1FD || pn == 0x1FE))
            mmc2_cb(pn);

        /* We're done with this tile, move onto the next one */
        if(xc != 31) {
            ++nt;
            ++xc;

            /* The attribute byte only changes once per 4 tiles */
            if(!(xc & 0x03)) {
                ++at;
                atb = ppu_map[at >> 8][(uint8)at];
            }
        }
        else {
            xc = 0;
            nt = (nt - 31) ^ 0x0400;
            at = (at - 7) ^ 0x0400;
            atb = ppu_map[at >> 8][(uint8)at];
        }
    }

    /* Blank the first 8 pixels if we're supposed to */
    if(!(ppu_regs[1] & 0x02)) {
        uint32 *bgpl = (uint32 *)(bg_pixels + 8);
        *bgpl++ = 0;
        *bgpl++ = 0;

        bgpl = (uint32 *)(sp + 8);
        *bgpl++ = 0;
        *bgpl++ = 0;
    }
}

#define SKIP_BG_PIXEL() { \
    *bgp++ = *pixels++; \
}

static void ppu_bg_skip(int line, uint8 bg_pixels[256 + 16]) {
    int xc, yf, nt;
    int col, pn, pth;
    uint8 ntb;
    nes_ppu_pattern_t *pat;
    uint8 *bgp = bg_pixels, *pixels;

    /* If sprite 0 is not on this line, then we can bail out now... */
    if(sprs[line][0].spr_num != 0)
        return;

    /* Which half of the pattern table are we using? */
    pth = (ppu_regs[0] & 0x10) << 4;

    /* Grab the X and Y scroll values from the "V register". */
    xc = ppu_v & 0x1F;
    yf = (ppu_v >> 12) & 0x07;

    /* Grab the name table's address */
    nt = 0x2000 | (ppu_v & 0x0FFF);

    /* Set up our pointer... */
    bgp += 8 - ppu_x;

    for(col = 0; col < 33; ++col) {
        /* Grab the name table byte for this tile and the pattern that it
           corresponds to. */
        ntb = ppu_map[nt >> 8][(uint8)nt];
        pn = ntb + pth;
        pat = ppu_patterns + pn;

        /* Update the cache if the pattern is marked as dirty */
        if(pat->dirty)
            nes_ppu_update_cache(pn);

        /* "Render" the pixels! */
        pixels = pat->texture[0] + (yf << 3);
        SKIP_BG_PIXEL();
        SKIP_BG_PIXEL();
        SKIP_BG_PIXEL();
        SKIP_BG_PIXEL();
        SKIP_BG_PIXEL();
        SKIP_BG_PIXEL();
        SKIP_BG_PIXEL();
        SKIP_BG_PIXEL();

        /* If we've got an MMC2 or an MMC4 and we need to set the latches, do
           so now. */
        if(mmc2_cb &&
           (pn == 0x0FD || pn == 0x0FE || pn == 0x1FD || pn == 0x1FE))
            mmc2_cb(pn);

        /* We're done with this tile, move onto the next one */
        if(xc != 31) {
            ++nt;
            ++xc;
        }
        else {
            xc = 0;
            nt = (nt - 31) ^ 0x0400;
        }
    }

    /* Blank the first 8 pixels if we're supposed to */
    if(!(ppu_regs[1] & 0x02)) {
        uint32 *bgpl = (uint32 *)(bg_pixels + 8);
        *bgpl++ = 0;
        *bgpl++ = 0;
    }
}

#define RENDER_SPR_PIXEL_FG_SPR0() { \
    pixel = *pixels++; \
    if(pixel) { \
        px[x] = pixel + pal; \
        col_tab[x] = 1; \
        if(bg_pixels[x]) \
            ppu_regs[2] |= 0x40; \
    } \
    ++x; \
}

#define RENDER_SPR_PIXEL_BG_SPR0() { \
    pixel = *pixels++; \
    if(pixel) { \
        col_tab[x] = 1; \
        if(bg_pixels[x]) { \
            ppu_regs[2] |= 0x40; \
        } \
        else { \
            px[x] = pixel + pal; \
        } \
    } \
    ++x; \
}

#define RENDER_SPR_PIXEL_FG() { \
    pixel = *pixels++; \
    if(!col_tab[x] && pixel) { \
        px[x] = pixel + pal; \
        col_tab[x] = 1; \
    } \
    ++x; \
}

#define RENDER_SPR_PIXEL_BG() { \
    pixel = *pixels++; \
    if(!col_tab[x] && pixel) { \
        col_tab[x] = 1; \
        if(!bg_pixels[x]) { \
            px[x] = pixel + pal; \
        } \
    } \
    ++x; \
}

static void ppu_spr_draw(int line, uint8 *px, uint8 bg_pixels[256 + 16]) {
    static uint8 col_tab[256 + 16];
    int i = 0, x, pal, pn;
    nes_ppu_pattern_t *pat;
    uint8 *pixels;
    uint8 pixel;

    /* First of all, clear out our colision table */
    memset(col_tab, 0, 256 + 16);

    /* See if sprite 0 is on this line, since it needs a bit of special handling
       in how we deal with it. */
    if(sprs[line][0].spr_num == 0) {
        i = 1;
        pn = sprs[line][0].pattern;
        pat = ppu_patterns + pn;

        /* Update the cache if the pattern is marked as dirty */
        if(pat->dirty)
            nes_ppu_update_cache(pn);

        pixels = &pat->texture[sprs[line][0].tex][sprs[line][0].y << 3];
        x = sprs[line][0].x + 8;
        pal = sprs[line][0].pal + 16;

        if(sprs[line][0].bg) {
            RENDER_SPR_PIXEL_BG_SPR0();
            RENDER_SPR_PIXEL_BG_SPR0();
            RENDER_SPR_PIXEL_BG_SPR0();
            RENDER_SPR_PIXEL_BG_SPR0();
            RENDER_SPR_PIXEL_BG_SPR0();
            RENDER_SPR_PIXEL_BG_SPR0();
            RENDER_SPR_PIXEL_BG_SPR0();
            RENDER_SPR_PIXEL_BG_SPR0();
        }
        else {
            RENDER_SPR_PIXEL_FG_SPR0();
            RENDER_SPR_PIXEL_FG_SPR0();
            RENDER_SPR_PIXEL_FG_SPR0();
            RENDER_SPR_PIXEL_FG_SPR0();
            RENDER_SPR_PIXEL_FG_SPR0();
            RENDER_SPR_PIXEL_FG_SPR0();
            RENDER_SPR_PIXEL_FG_SPR0();
            RENDER_SPR_PIXEL_FG_SPR0();
        }

        /* If we've got an MMC2 or an MMC4 and we need to set the latches, do
           so now. */
        if(mmc2_cb &&
           (pn == 0x0FD || pn == 0x0FE || pn == 0x1FD || pn == 0x1FE))
            mmc2_cb(pn);
    }

    /* Go through all the sprites on this line. */
    for(; i < 8 && sprs[line][i].spr_num != 0xFF; ++i) {
        pn = sprs[line][i].pattern;
        pat = ppu_patterns + pn;

        /* Update the cache if the pattern is marked as dirty */
        if(pat->dirty)
            nes_ppu_update_cache(pn);

        pixels = &pat->texture[sprs[line][i].tex][sprs[line][i].y << 3];
        x = sprs[line][i].x + 8;
        pal = sprs[line][i].pal + 16;

        if(sprs[line][i].bg) {
            RENDER_SPR_PIXEL_BG();
            RENDER_SPR_PIXEL_BG();
            RENDER_SPR_PIXEL_BG();
            RENDER_SPR_PIXEL_BG();
            RENDER_SPR_PIXEL_BG();
            RENDER_SPR_PIXEL_BG();
            RENDER_SPR_PIXEL_BG();
            RENDER_SPR_PIXEL_BG();
        }
        else {
            RENDER_SPR_PIXEL_FG();
            RENDER_SPR_PIXEL_FG();
            RENDER_SPR_PIXEL_FG();
            RENDER_SPR_PIXEL_FG();
            RENDER_SPR_PIXEL_FG();
            RENDER_SPR_PIXEL_FG();
            RENDER_SPR_PIXEL_FG();
            RENDER_SPR_PIXEL_FG();
        }

        /* If we've got an MMC2 or an MMC4 and we need to set the latches, do
           so now. */
        if(mmc2_cb &&
           (pn == 0x0FD || pn == 0x0FE || pn == 0x1FD || pn == 0x1FE))
            mmc2_cb(pn);
    }

    /* Set the sprite overflow bit, if appropriate. */
    if(sprs_per_line[line] > 8)
        ppu_regs[2] |= 0x20;
}

#define SKIP_SPR_PIXEL_SPR0() { \
    if(*pixels++ && bg_pixels[x]) \
        ppu_regs[2] |= 0x40; \
    ++x; \
}

static void ppu_spr_skip(int line, uint8 bg_pixels[256 + 16]) {
    int x;
    nes_ppu_pattern_t *pat;
    uint8 *pixels;
    int pn, i = 0;

    /* See if sprite 0 is on this line, since its really the only sprite we care
       about for now... */
    if(sprs[line][0].spr_num == 0) {
        i = 1;
        pn = sprs[line][0].pattern;
        pat = ppu_patterns + pn;

        /* Update the cache if the pattern is marked as dirty */
        if(pat->dirty)
            nes_ppu_update_cache(pn);

        pixels = &pat->texture[sprs[line][0].tex][sprs[line][0].y << 3];
        x = sprs[line][0].x + 8;

        SKIP_SPR_PIXEL_SPR0();
        SKIP_SPR_PIXEL_SPR0();
        SKIP_SPR_PIXEL_SPR0();
        SKIP_SPR_PIXEL_SPR0();
        SKIP_SPR_PIXEL_SPR0();
        SKIP_SPR_PIXEL_SPR0();
        SKIP_SPR_PIXEL_SPR0();
        SKIP_SPR_PIXEL_SPR0();

        /* If we've got an MMC2 or an MMC4 and we need to set the latches, do
           so now. */
        if(mmc2_cb &&
           (pn == 0x0FD || pn == 0x0FE || pn == 0x1FD || pn == 0x1FE))
            mmc2_cb(pn);
    }

    /* We only have to look at the other sprites if we've got either an MMC2 or
       an MMC4... */
    if(mmc2_cb) {
        for(; i < 8 && sprs[line][i].spr_num != 0xFF; ++i) {
            pn = sprs[line][i].pattern;

            if(pn == 0x0FD || pn == 0x0FE || pn == 0x1FD || pn == 0x1FE)
                mmc2_cb(pn);
        }
    }

    /* Set the sprite overflow bit, if appropriate. */
    if(sprs_per_line[line] > 8)
        ppu_regs[2] |= 0x20;
}

void nes_ppu_execute(int line, int skip) {
    int bg = ppu_regs[1] & 0x08, spr = ppu_regs[1] & 0x10, i;
    uint8 bg_pixels[256 + 16];
    uint8 fb_buf[256 + 16];
    uint8 *px = fb_buf;
    int tmp;

    /* Calculate all the sprite data for the frame on the first line */
    if(!line)
        ppu_calc_spr();

    /* Check if the background is disabled. If so, then fill the line with the
       0th color. Sprites might still be enabled, so we still do have to deal
       with the rest of the rendering from here. */
    if(!bg && !skip) {
        for(i = 0; i < 256 + 16; ++i) {
            *px++ = 0;
            bg_pixels[i] = 0;
        }
    }

    if(bg || spr) {
        if(!line)
            ppu_v = ppu_t;
        else
            ppu_v = (ppu_t & 0x041F) | (ppu_v & 0xFBE0);

        if(!skip) {
            if(bg)
                ppu_bg_draw(line, px, bg_pixels);

            if(spr)
                ppu_spr_draw(line, px, bg_pixels);
        }
        else {
            if(bg)
                ppu_bg_skip(line, bg_pixels);

            if(spr)
                ppu_spr_skip(line, bg_pixels);
        }

        /* Update V for the next line */
        if(((ppu_v >> 12) & 0x07) == 0x07) {
            tmp = (ppu_v >> 5) & 0x1F;
    
            /* See if we're at row 29... */
            if(tmp == 29) {
                /* Yes, we are. So, switch to the other nametable (as the
                   mirroring mode directs). */
                ppu_v ^= 0x0800;
                ppu_v &= (ppu_v & 0x0C1F);
            }
            /* Next check if we're at row 31... */
            else if(tmp == 31) {
                /* In this case, we don't actually flip bit 11. */
                ppu_v &= 0x041F;
            }
            /* And, for the easy cases... Just move to the next row. */
            else {
                ppu_v = (ppu_v & 0x0FFF) + 0x0020;
            }
        }
        else {
            /* Easy case, just move to the next line. We aren't moving to the
               next row of tiles. */
            ppu_v += 0x1000;
        }
    }

    /* Copy over the buffer to the actual framebuffer. */
    if(!skip) {
        uint8 pal[32];
        pixel_t *px2 = ppu_framebuffer + (line << 8);
        int emph = (ppu_regs[1] & 0xE0) << 1;

        nes_ppu_fetch_bg_pal(pal);
        nes_ppu_fetch_spr_pal(pal + 16);

        for(i = 0; i < 256; ++i) {
            px2[i] = nes_pal[pal[fb_buf[i + 8]] | emph];
        }
    }
}

int nes_ppu_init(void) {
    int i;

    /* Init everything... */
    memset(ppu_regs, 0, 8);
    memset(ppu_oam_ram, 0, 256);
    memset(ppu_bg_pal, 0, 16);
    memset(ppu_spr_pal, 0, 16);
    memset(ppu_nametables, 0, 4096);

    for(i = 0; i < 0x20; ++i) {
        ppu_map[i] = NULL;
    }

    /* Mark all patterns as dirty */
    for(i = 0; i < 512; ++i) {
        ppu_patterns[i].dirty = 1;
    }

    ppu_map[0x20] = ppu_nametables;
    ppu_map[0x21] = ppu_nametables + 0x100;
    ppu_map[0x22] = ppu_nametables + 0x200;
    ppu_map[0x23] = ppu_nametables + 0x300;
    ppu_map[0x24] = ppu_nametables;
    ppu_map[0x25] = ppu_nametables + 0x100;
    ppu_map[0x26] = ppu_nametables + 0x200;
    ppu_map[0x27] = ppu_nametables + 0x300;
    ppu_map[0x28] = ppu_nametables;
    ppu_map[0x29] = ppu_nametables + 0x100;
    ppu_map[0x2A] = ppu_nametables + 0x200;
    ppu_map[0x2B] = ppu_nametables + 0x300;
    ppu_map[0x2C] = ppu_nametables;
    ppu_map[0x2D] = ppu_nametables + 0x100;
    ppu_map[0x2E] = ppu_nametables + 0x200;
    ppu_map[0x2F] = ppu_nametables + 0x300;

    ppu_addr_latched = 0;
    ppu_v = 0;
    ppu_t = 0;
    ppu_x = 0;
    ppu_oam_addr = 0;
    ppu_patterns_rom = 0;
    mmc2_cb = NULL;

    for(i = 0; i < 256; ++i) {
        ppu_lut[i] = (i & 0x01) | ((i & 0x02) << 1) | ((i & 0x04) << 2) |
            ((i & 0x08) << 3) | ((i & 0x10) << 4) | ((i & 0x20) << 5) |
            ((i & 0x40) << 6) | ((i & 0x80) << 7);
    }

#ifndef _arch_dreamcast
    if(!(ppu_framebuffer = (pixel_t *)malloc(256 * 256 * sizeof(pixel_t))))
        return -1;
#else
    ppu_framebuffer = (pixel_t *)memalign(32, 256 * 256 * sizeof(pixel_t));
    if(!ppu_framebuffer)
        return -1;

    /* Put the framebuffer in P2 so that it is not cacheable. */
    ppu_framebuffer = (pixel_t *)((uint32)ppu_framebuffer | 0xA0000000);
#endif

    memset(ppu_framebuffer, 0, 256 * 256 * sizeof(pixel_t));
    gui_set_aspect(16.0f, 15.0f);

    return 0;
}

int nes_ppu_reset(void) {
    int i;

    /* Init everything... */
    memset(ppu_regs, 0, 8);
    memset(ppu_oam_ram, 0, 256);
    memset(ppu_bg_pal, 0, 16);
    memset(ppu_spr_pal, 0, 16);
    memset(ppu_nametables, 0, 4096);

    /* Mark all patterns as dirty */
    for(i = 0; i < 512; ++i) {
        ppu_patterns[i].dirty = 1;
    }

    ppu_addr_latched = 0;
    ppu_v = 0;
    ppu_t = 0;
    ppu_x = 0;
    ppu_oam_addr = 0;
    ppu_patterns_rom = 0;
    mmc2_cb = NULL;

    memset(ppu_framebuffer, 0, 256 * 256 * sizeof(pixel_t));
    gui_set_aspect(16.0f, 15.0f);

    return 0;
}

void nes_ppu_set_mmc2(void (*cb)(uint16 pn)) {
    mmc2_cb = cb;
}

int nes_ppu_shutdown(void) {
#ifdef _arch_dreamcast
    /* Put the framebuffer back into P1. */
    ppu_framebuffer = (pixel_t *)((uint32)ppu_framebuffer & 0xDFFFFFFF);
#endif

    free(ppu_framebuffer);
    return 0;
}

static int nes_ppu_write_cram_context(FILE *fp) {
    uint8 data[4];

    data[0] = 'C';
    data[1] = 'R';
    data[2] = 'A';
    data[3] = 'M';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(48, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(real_bg_pal, 1, 16, fp);
    fwrite(real_spr_pal, 1, 16, fp);

    return 0;
}

static int nes_ppu_read_cram_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 48)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    memcpy(real_bg_pal, buf + 16, 16);
    memcpy(ppu_bg_pal, buf + 16, 16);
    memcpy(real_spr_pal, buf + 32, 16);
    memcpy(ppu_spr_pal, buf + 32, 16);
    return 0;
}

static int nes_ppu_write_oamr_context(FILE *fp) {
    uint8 data[4];

    data[0] = 'O';
    data[1] = 'A';
    data[2] = 'M';
    data[3] = 'R';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(272, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(ppu_oam_ram, 1, 256, fp);

    return 0;
}

static int nes_ppu_read_oamr_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 272)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    memcpy(ppu_oam_ram, buf + 16, 256);
    return 0;
}

static int nes_ppu_write_ntrm_context(FILE *fp) {
    uint8 data[4];

    data[0] = 'N';
    data[1] = 'T';
    data[2] = 'R';
    data[3] = 'M';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(4112, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(ppu_nametables, 1, 4096, fp);

    return 0;
}

static int nes_ppu_read_ntrm_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 4112)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    memcpy(ppu_nametables, buf + 16, 4096);
    return 0;
}

int nes_ppu_write_context(FILE *fp) {
    uint8 data[4];

    data[0] = 'N';
    data[1] = 'P';
    data[2] = 'P';
    data[3] = 'U';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(4464, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    UINT32_TO_BUF(16, data);
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(ppu_regs, 1, 8, fp);

    UINT16_TO_BUF(ppu_t, data);
    fwrite(data, 1, 2, fp);
    fwrite(&ppu_x, 1, 1, fp);
    fwrite(&ppu_oam_addr, 1, 1, fp);

    data[0] = (ppu_map[0x20] - ppu_nametables) >> 10;
    data[1] = (ppu_map[0x24] - ppu_nametables) >> 10;
    data[2] = (ppu_map[0x28] - ppu_nametables) >> 10;
    data[3] = (ppu_map[0x2C] - ppu_nametables) >> 10;
    fwrite(data, 1, 4, fp);

    /* Write out the children */
    if(nes_ppu_write_cram_context(fp)) {
        return -1;
    }
    else if(nes_ppu_write_oamr_context(fp)) {
        return -1;
    }
    else if(nes_ppu_write_ntrm_context(fp)) {
        return -1;
    }

    return 0;
}

int nes_ppu_read_context(const uint8 *buf) {
    uint32 len, child, clen;
    uint16 ver;
    const uint8 *ptr;
    int rv, i;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 4464)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    BUF_TO_UINT32(buf + 12, child);
    if(child != 16)
        return -1;

    /* Read the PPU state */
    for(i = 0; i < 8; ++i) {
        nes_ppu_writereg(i, buf[16 + i]);
    }

    BUF_TO_UINT16(buf + 24, ppu_t);
    ppu_v = ppu_t;
    ppu_x = buf[26];
    ppu_oam_addr = buf[27];
    nes_ppu_set_tblmirrors(buf[28], buf[29], buf[30], buf[31]);
    ppu_addr_latched = 0;

    /* Handle child nodes */
    ptr = buf + 16 + child;

    while(ptr < buf + len) {
        rv = 0;

        /* Check for a malformed child node */
        if(ptr + 16 > buf + len)
            return -2;

        child = FOURCC_TO_UINT32(ptr[0], ptr[1], ptr[2], ptr[3]);
        BUF_TO_UINT32(ptr + 4, clen);

        /* Make sure the length is sane */
        if(ptr + clen > buf + len)
            return -2;

        switch(child) {
            case FOURCC_TO_UINT32('O', 'A', 'M', 'R'):
                rv = nes_ppu_read_oamr_context(ptr);
                break;

            case FOURCC_TO_UINT32('C', 'R', 'A', 'M'):
                rv = nes_ppu_read_cram_context(ptr);
                break;

            case FOURCC_TO_UINT32('N', 'T', 'R', 'M'):
                rv = nes_ppu_read_ntrm_context(ptr);
                break;

            default:
                /* See if its marked as essential... */
                BUF_TO_UINT16(ptr + 10, ver);
                if(ver & 1)
                    rv = -1;
        }

        if(rv)
            return rv;

        ptr += clen;
    }

    /* Mark all patterns as dirty */
    for(i = 0; i < 512; ++i) {
        ppu_patterns[i].dirty = 1;
        /* There's no use in clearing the textures, since the will be
           overwritten before they're used, anyway */
    }

    return 0;
}
