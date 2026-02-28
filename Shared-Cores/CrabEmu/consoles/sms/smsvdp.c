/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2006, 2007, 2008, 2012, 2013, 2014, 2016 Lawrence Sebald

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

#include "sms.h"
#include "smsvdp.h"
#include "tms9918a.h"
#include "smsz80.h"
#include "smsvcnt.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _arch_dreamcast
#include <malloc.h>
#endif

sms_vdp_t smsvdp;
static uint32 lut[256];

/* Size of the top border for each video mode, in scanlines. */
static const int top_border[6] = { 27, 11, 0, 54, 38, 30 };

static const int kyoukai1[] = { 216, 232, 240, 240, 256, 264 };
static const int kyoukai2[] = { 235, 251, 262, 259, 275, 283 };

#ifdef CRABEMU_32BIT_COLOR
static void update_local_pal_sms(int num) {
    /* Calculate the RGB888 color from the BGR222 color */
    smsvdp.pal[num] = ((smsvdp.cram[num] & 0x03) << 22) |
                      ((smsvdp.cram[num] & 0x0C) << 12) |
                      ((smsvdp.cram[num] & 0x30) << 2) |
                      (0xFF000000);
}

static void update_local_pal_gg(int num) {
    /* Calculate the RGB888 color from the BGR444 color */
    smsvdp.pal[num >> 1] = ((smsvdp.cram[num] & 0x0F) << 20) |
                           ((smsvdp.cram[num] & 0xF0) << 8) |
                           ((smsvdp.cram[num + 1] & 0x0F) << 4) |
                           (0xFF000000);
}
#else
static void update_local_pal_sms(int num) {
    /* Calculate the RGB555 color from the BGR222 color */
    smsvdp.pal[num] = ((smsvdp.cram[num] & 0x03) << 13) |
                      ((smsvdp.cram[num] & 0x0C) << 6) |
                      ((smsvdp.cram[num] & 0x30) >> 1);
}

static void update_local_pal_gg(int num) {
    /* Calculate the RGB555 color from the BGR444 color */
    smsvdp.pal[num >> 1] = ((smsvdp.cram[num] & 0x0F) << 11) |
                           ((smsvdp.cram[num] & 0xF0) << 2) |
                           ((smsvdp.cram[num + 1] & 0x0F) << 1);
}
#endif

static __INLINE__ void readjust_name_table() {
    if(smsvdp.lines == 192) {
        smsvdp.name_table = smsvdp.vram + ((smsvdp.regs[2] & 0x0E) << 10);
    }
    else {
        smsvdp.name_table = smsvdp.vram + ((smsvdp.regs[2] & 0x0C) << 10) +
            0x700;
    }
}

typedef void (*smsvdp_draw_func)(int line, pixel_t *px);
typedef void (*smsvdp_skip_func)(int line);

static void dummy_draw(int line __UNUSED__, pixel_t *px __UNUSED__) {
}

static void dummy_skip(int line __UNUSED__) {
}

static smsvdp_draw_func bg_draw = &dummy_draw;
static smsvdp_draw_func spr_draw = &dummy_draw;
static smsvdp_skip_func spr_skip = &dummy_skip;

static const uint8 *vcnt_tab = vcnt_ntsc_192;

void sms_vdp_update_cache(int pat) {
    uint8 *bitplane;
    int i, j;
    uint8 pixel;
    uint8 *tex1, *tex2, *tex3, *tex4;
    int tmp1, tmp2, tmp3, tmp4;
    uint32 lutval;

    if(pat >= 512 || !smsvdp.pattern[pat].dirty)
        return;

    /* Determine where we should start converting from and to... */
    bitplane = smsvdp.vram + (pat << 5);
    tex1 = smsvdp.pattern[pat].texture[0] + 7;
    tex2 = smsvdp.pattern[pat].texture[1];
    tex3 = smsvdp.pattern[pat].texture[2] + 63;
    tex4 = smsvdp.pattern[pat].texture[3] + 56;
    
    for(i = 0; i < 8; ++i) {
        tmp1 = *bitplane++;
        tmp2 = *bitplane++;
        tmp3 = *bitplane++;
        tmp4 = *bitplane++;

        /* lutval = 8x4bpp pixels */
        lutval = (lut[tmp1]) | (lut[tmp2] << 1) | (lut[tmp3] << 2) |
            (lut[tmp4] << 3);

        for(j = 0; j < 8; ++j) {
            pixel = lutval & 0x0F;

            *tex1-- = pixel;
            *tex2++ = pixel;
            *tex3-- = pixel;
            *tex4++ = pixel;

            /* Shift off the pixel we just looked at */
            lutval >>= 4;
        }

        tex1 += 16;
        tex4 -= 16;
    }

    smsvdp.pattern[pat].dirty = 0;
}

#define DRAW_PIXEL_HIGH() { \
    entry = (*pixels++) + pal; \
    *px++ = smsvdp.pal[entry]; \
    *current_alpha++ = (entry & 0x0F); \
}

#define DRAW_PIXEL_LOW() { \
    entry = (*pixels++) + pal; \
    *px++ = smsvdp.pal[entry]; \
    *current_alpha++ = 0; \
}

static void sms_vdp_m4_draw_bg(int line, pixel_t *px) {
    int row, col, tile, tilenum, tmp;
    int rendercol, renderrow, xoff, yoff;
    uint8 *pixels;
    int entry, pal, attrib;
    uint16 *name_table_short;
    uint8 *current_alpha;

    row = line >> 3;

    /* Figure out what column/row to render first */
    if(row < 2 && smsvdp.regs[0] & 0x40) {
        /* Do not scroll the top two rows, if requested. */
        rendercol = 0;
        xoff = 0;
    }
    else {
        rendercol = smsvdp.xscroll_coarse;
        xoff = smsvdp.xscroll_fine;
    }

    if(smsvdp.lines == 192) {
        tmp = line + smsvdp.regs[9];

        if(tmp > 223)
            tmp -= 224;
    }
    else {
        tmp = (line + smsvdp.regs[9]) & 0xFF;
    }

    yoff = smsvdp.yscroll_fine;

    renderrow = (tmp >> 3);

    name_table_short = &((uint16 *)smsvdp.name_table)[renderrow << 5];
    px += xoff;
    current_alpha = smsvdp.alpha + xoff;

    /* Render each pixel of background data for this line */
    for(col = 0; col < 32; ++col) {
        if(col == 24 && smsvdp.regs[0] & 0x80) {
            /* Disable vertical scrolling for the last few columns */
            renderrow = line >> 3;
            yoff = 0;
            name_table_short = &((uint16 *)smsvdp.name_table)[renderrow << 5];
        }

        /* Get the tile that we're rendering */
        tile = name_table_short[rendercol];
#ifdef __BIG_ENDIAN__
        tile = ((tile & 0xFF) << 8) | ((tile >> 8) & 0xFF);
#endif
        tilenum = tile & 0x1FF;

        /* Check if the pattern is dirtied, if so, clean it up */
        if(smsvdp.pattern[tilenum].dirty)
            sms_vdp_update_cache(tilenum);

        attrib = (tile >> 9) & 0x03;

        /* Find the texture */
        pixels = &smsvdp.pattern[tilenum].texture[attrib]
            [((line + yoff) & 0x07) << 3];

        pal = (tile & 0x800) >> 7;

        /* Draw the pixels */
        if(tile & 0x1000) {
            DRAW_PIXEL_HIGH();
            DRAW_PIXEL_HIGH();
            DRAW_PIXEL_HIGH();
            DRAW_PIXEL_HIGH();
            DRAW_PIXEL_HIGH();
            DRAW_PIXEL_HIGH();
            DRAW_PIXEL_HIGH();
            DRAW_PIXEL_HIGH();
        }
        else {
            DRAW_PIXEL_LOW();
            DRAW_PIXEL_LOW();
            DRAW_PIXEL_LOW();
            DRAW_PIXEL_LOW();
            DRAW_PIXEL_LOW();
            DRAW_PIXEL_LOW();
            DRAW_PIXEL_LOW();
            DRAW_PIXEL_LOW();
        }

        rendercol = (rendercol + 1) & 0x1F;
    }
}

#undef DRAW_PIXEL_HIGH
#undef DRAW_PIXEL_LOW

static void sms_vdp_m4_draw_spr(int line, pixel_t *px) {
    uint8 *sat = smsvdp.sat;
    uint8 *pix;
    static uint8 col_tab[256];
    int height, i, n = 0, j, x, y, num, half, ds = 0, tiled = 0, ls = 0;
    pixel_t *pal = (smsvdp.pal + 0x10);
    pixel_t *current_pixel;

    /* First of all, clear out our colision table */
    memset(col_tab, 0, 0xFF);

    /* Normal sprites are 8 pixels high, so start with that */
    height = 8;

    /* If bit 1 of register 1 is set, then the height is 16 */
    if(smsvdp.regs[1] & 0x02) {
        tiled = 1;
        height = 16;
    }

    /* If bit 0 of register 1 is set, sprites have their pixels doubled */
    if(smsvdp.regs[1] & 0x01) {
        ds = 1;
        height <<= 1;
    }

    /* If bit 3 of register 0 is set, shift sprites left 8 pixels */
    ls = smsvdp.regs[0] & 0x08;

    /* Which half of the pattern table are we getting the patterns from? */
    half = (smsvdp.regs[6] & 0x04) << 6;

    /* Parse the table */
    for(i = 0; i < 64; ++i) {
        /* Check for the end of list flag */
        if(sat[i] == 0xD0) {
            break;
        }

        y = sat[i] + 1;

        /* sat[i] = 255 implies that the sprite should start on line 0. */
        if(y == 256)
            y = 0;

        /* Check the position of this sprite */
        if(line >= y && line <= y + height - 1) {
            /* If its on this line, make sure that we haven't already rendered
               8 sprites */
            if(n < 8) {
                /* Determine the X position, which is stored in
                   sat[i * 2 + 0x80], and can be changed by bit 3 of register 0,
                   also get the pattern number, which is stored in
                   sat[i * 2 + 0x81] */
                x = sat[(i << 1) | 0x80] - ls;
                num = sat[(i << 1) | 0x81] + half;

                if(tiled)
                    num &= 0x1FE;

                /* Set our pixel pointer up */
                if(((line - y) >> ds) > 7) {
                    if(smsvdp.pattern[num + 1].dirty)
                        sms_vdp_update_cache(num + 1);
                    pix = &smsvdp.pattern[num + 1].texture[0][(((line - y) >>
                                                                ds) - 8) << 3];
                }
                else {
                    if(smsvdp.pattern[num].dirty)
                        sms_vdp_update_cache(num);
                    pix = &smsvdp.pattern[num].texture[0][((line - y) >> ds) <<
                                                          3];
                }

                /* Don't draw to a negative coordinate */
                j = 0;
                if(x < 0) {
                    j = 0 - x;
                    x = 0;
                }

                current_pixel = px + x;

                for(; j < 8 && x < 256; ++j, ++x, ++current_pixel) {
                    if(pix[j]) {
                        /* Check for collision, set the flag if there is one */
                        if(col_tab[x]) {
                            smsvdp.status |= 0x20;
                            goto doublesize;
                        }

                        /* Make sure not to tromp on a high-priority background
                           tile */
                        if(smsvdp.alpha[x])
                            goto doublesize;

                        /* Actually draw the pixel */
                        *current_pixel = pal[pix[j]];
                        col_tab[x] = 1;

doublesize:
                        if(ds) {
                            ++current_pixel;

                            /* Check for collision, set the flag if there is
                               one */
                            if(col_tab[++x]) {
                                smsvdp.status |= 0x20;
                                continue;
                            }

                            /* Make sure not to tromp on a high-priority
                               background tile */
                            if(smsvdp.alpha[x])
                                continue;

                            /* Actually draw the pixel */
                            *current_pixel = pal[pix[j]];
                            col_tab[x] = 1;
                        }
                    }
                }
                ++n;
            }
            else {
                /* Too many sprites on this line, set the flag, and stop
                   rendering */
                smsvdp.status |= 0x40;
                break;
            }
        }
    }
}

static void sms_vdp_m4_skip_spr(int line) {
    static uint8 col_tab[256];
    uint8 *sat = smsvdp.sat;
    uint8 *pix;
    int height, i, n = 0, j, x, y, num, half, ds = 0, tiled = 0, ls = 0;

    /* See if all the flags we can affect are already set. If so, we don't need
       to go any further. */
    if((smsvdp.status & 0x60) == 0x60)
        return;

    /* First of all, clear out our colision table */
    memset(col_tab, 0, 0xFF);

    /* Normal sprites are 8 pixels high, so start with that */
    height = 8;

    /* If bit 1 of register 1 is set, then the height is 16 */
    if(smsvdp.regs[1] & 0x02) {
        tiled = 1;
        height = 16;
    }

    /* If bit 0 of register 1 is set, sprites have their pixels doubled */
    if(smsvdp.regs[1] & 0x01) {
        ds = 1;
        height <<= 1;
    }

    /* If bit 3 of register 0 is set, shift sprites left 8 pixels */
    ls = smsvdp.regs[0] & 0x08;

    /* Which half of the pattern table are we getting the patterns from? */
    half = (smsvdp.regs[6] & 0x04) << 6;

    /* Parse the table */
    for(i = 0; i < 64; ++i) {
        /* Check for the end of list flag */
        if(sat[i] == 0xD0)
            return;

        y = sat[i] + 1;

        /* Check the position of this sprite */
        if(line >= y && line <= y + height - 1) {
            /* If its on this line, make sure that we haven't already rendered
               8 sprites */
            if(n < 8) {
                /* Determine the X position, which is stored in
                   sat[i * 2 + 0x80], and can be changed by bit 3 of register 0,
                   also get the pattern number, which is stored in
                   sat[i * 2 + 0x81] */
                x = sat[(i << 1) | 0x80] - ls;
                num = sat[(i << 1) | 0x81] + half;

                if(tiled)
                    num &= 0x1FE;

                /* Set our pixel pointer up. Sadly, even when skipping frames,
                   we might have to update the tile cache... */
                if(((line - y) >> ds) > 7) {
                    if(smsvdp.pattern[num + 1].dirty)
                        sms_vdp_update_cache(num + 1);
                    pix = &smsvdp.pattern[num + 1].texture[0][(((line - y) >>
                                                                ds) - 8) << 3];
                }
                else {
                    if(smsvdp.pattern[num].dirty)
                        sms_vdp_update_cache(num);
                    pix = &smsvdp.pattern[num].texture[0][((line - y) >> ds) <<
                                                          3];
                }

                /* Don't draw to a negative coordinate */
                j = 0;
                if(x < 0) {
                    j = 0 - x;
                    x = 0;
                }

                for(; j < 8 && x < 256; ++j, ++x) {
                    if(pix[j]) {
                        /* Check for collision, set the flag if there is one */
                        if(col_tab[x]++)
                            smsvdp.status |= 0x20;

                        if(ds) {
                            /* Check for collision, set the flag if there is
                               one */
                            if(col_tab[++x]++)
                                smsvdp.status |= 0x20;
                        }
                    }
                }
                ++n;
            }
            else {
                /* Too many sprites on this line, set the flag, and we're done
                   with sprites... */
                smsvdp.status |= 0x40;
                return;
            }
        }
    }
}

uint32 sms_vdp_execute(int line, int skip) {
    int i;
    uint32 cycles = 0;

    /* Draw only if the display is enabled */
    if(smsvdp.regs[1] & 0x40 && line < smsvdp.lines) {
        if(!skip) {
            pixel_t *px;

#ifndef _arch_dreamcast
            if(smsvdp.borders)
                px = (smsvdp.framebuffer_base) + (line << smsvdp.fb_x);
            else
#endif /* !_arch_dreamcast */
                px = (smsvdp.framebuffer) + (line << smsvdp.fb_x);

#ifndef _arch_dreamcast
            /* Fill in the left border, if we're bothering to emulate them.
               This will also, conveniently, reposition the px pointer to beyond
               the border for us. */
            if(smsvdp.borders) {
                int tmp = (smsvdp.regs[7] & 0x0F) | 0x10;
                pixel_t col = smsvdp.pal[tmp];

                /* The left border is 13 pixels in size. */
                *px++ = col;    /* 1 */
                *px++ = col;    /* 2 */
                *px++ = col;    /* 3 */
                *px++ = col;    /* 4 */
                *px++ = col;    /* 5 */
                *px++ = col;    /* 6 */
                *px++ = col;    /* 7 */
                *px++ = col;    /* 8 */
                *px++ = col;    /* 9 */
                *px++ = col;    /* 10 */
                *px++ = col;    /* 11 */
                *px++ = col;    /* 12 */
                *px++ = col;    /* 13 */
            }
#endif /* !_arch_dreamcast */

            bg_draw(line, px);
            spr_draw(line, px);

            /* Determine if we should mask column 0, and if so, do it */
            if(smsvdp.regs[0] & 0x20 && !skip) {
                int tmp = (smsvdp.regs[7] & 0x0F) | 0x10;
                pixel_t col = smsvdp.pal[tmp];

                /* Each column is 8 pixels. Mask all 8 of them */
                *px++ = col;    /* 1 */
                *px++ = col;    /* 2 */
                *px++ = col;    /* 3 */
                *px++ = col;    /* 4 */
                *px++ = col;    /* 5 */
                *px++ = col;    /* 6 */
                *px++ = col;    /* 7 */
                *px++ = col;    /* 8 */
            }

#ifndef _arch_dreamcast
            /* Fill in the right border, if we're bothering to emulate them. */
            if(smsvdp.borders) {
                int tmp = (smsvdp.regs[7] & 0x0F) | 0x10;
                pixel_t col = smsvdp.pal[tmp];
                px = (smsvdp.framebuffer_base) + (line << smsvdp.fb_x) + 256 +
                    13;

                /* The right border is 15 pixels in size. */
                *px++ = col;    /* 1 */
                *px++ = col;    /* 2 */
                *px++ = col;    /* 3 */
                *px++ = col;    /* 4 */
                *px++ = col;    /* 5 */
                *px++ = col;    /* 6 */
                *px++ = col;    /* 7 */
                *px++ = col;    /* 8 */
                *px++ = col;    /* 9 */
                *px++ = col;    /* 10 */
                *px++ = col;    /* 11 */
                *px++ = col;    /* 12 */
                *px++ = col;    /* 13 */
                *px++ = col;    /* 14 */
                *px++ = col;    /* 15 */
            }
#endif /* !_arch_dreamcast */
        }
        else {
            /* Backgrounds can't actually affect anything status-wise, so there
               isn't any reason to do anything with them at all when skipping
               the frame. */
            spr_skip(line);
        }
    }
    else if(line < smsvdp.lines && !skip) {
        pixel_t *px;
#ifndef _arch_dreamcast
        if(smsvdp.borders)
            px = (smsvdp.framebuffer_base) + (line << smsvdp.fb_x);
        else
#endif /* !_arch_dreamcast */
            px = (smsvdp.framebuffer) + (line << smsvdp.fb_x);

        /* Blank the whole scanline. */
        int tmp = (smsvdp.regs[7] & 0x0F) | 0x10;
        pixel_t col = smsvdp.pal[tmp];

        for(i = 0; i < 256; ++i) {
            *px++ = col;
        }

#ifndef _arch_dreamcast
        /* Blank the rest of the scanline as well. */
        if(smsvdp.borders) {
            for(i = 0; i < 28; ++i) {
                *px++ = col;
            }
        }
#endif /* !_arch_dreamcast */
    }
#ifndef _arch_dreamcast
    /* If we're emulating borders, then we might have work to do outside the
       active display period. */
    else if(smsvdp.borders) {
        int bb = 0, tb = 512;

        if(smsvdp.vidmode == SMS_VIDEO_NTSC) {
            switch(smsvdp.lines) {
                case 192:
                    bb = kyoukai1[0];
                    tb = kyoukai2[0];
                    break;

                case 224:
                    bb = kyoukai1[1];
                    tb = kyoukai2[1];
                    break;

                case 240:
                    bb = kyoukai1[2];
                    tb = kyoukai2[2];
                    break;
            }
        }
        else {
            switch(smsvdp.lines) {
                case 192:
                    bb = kyoukai1[3];
                    tb = kyoukai2[3];
                    break;

                case 224:
                    bb = kyoukai1[4];
                    tb = kyoukai2[4];
                    break;

                case 240:
                    bb = kyoukai1[5];
                    tb = kyoukai2[5];
                    break;
            }
        }

        if(line < bb) {
            pixel_t *px = (smsvdp.framebuffer_base) + (line << smsvdp.fb_x);
            int tmp = (smsvdp.regs[7] & 0x0F) | 0x10;
            pixel_t col = smsvdp.pal[tmp];

            for(i = 0; i < 284; ++i) {
                *px++ = col;
            }
        }
        else if(line >= tb) {
            pixel_t *px = (smsvdp.framebuffer) + ((line - tb) << smsvdp.fb_x);
            int tmp = (smsvdp.regs[7] & 0x0F) | 0x10;
            pixel_t col = smsvdp.pal[tmp];

            for(i = 0; i < 284; ++i) {
                *px++ = col;
            }
        }
    }
#endif /* !_arch_dreamcast */

    if(line <= smsvdp.lines) {
        if(smsvdp.linecnt == 0) {
            /* The Register 10 line count will underflow, interrupt if
               enabled */
            smsvdp.linecnt = smsvdp.regs[10];

            if(smsvdp.regs[0] & 0x10) {
                smsvdp.flags |= SMS_VDP_FLAG_LINE_INT;
                cycles += sms_z80_run(1);
                sms_z80_assert_irq();
            }
        }
        else {
            /* Decrement the line counter */
            --smsvdp.linecnt;
        }
    }
    else {
        /* Reload the line counter */
        smsvdp.linecnt = smsvdp.regs[10];
    }

    if(line == smsvdp.lines + 1) {
        /* Update the vertical scroll value, if needed */
        if(smsvdp.flags & SMS_VDP_FLAG_VSCROLL_CHG) {
            smsvdp.regs[9] = smsvdp.v_scroll;

            smsvdp.yscroll_fine = smsvdp.v_scroll & 0x07;
            smsvdp.flags &= (~SMS_VDP_FLAG_VSCROLL_CHG);
        }

        /* Set the vblank flag, and signal an IRQ, if desired */
        smsvdp.status |= 0x80;

        if(smsvdp.regs[1] & 0x20) {
            /* This should make Chicago Syndicate work. */
            cycles += sms_z80_run(1);
            sms_z80_assert_irq();
        }
    }

    smsvdp.line = line;

    return cycles;
}

uint32 tms9918a_vdp_execute(int line, void (*irqfunc)(), int skip) {
    int i;

    /* Draw only if the display is enabled */
    if((smsvdp.regs[1] & 0x40) && line < 192) {
        if(!skip) {
            pixel_t *px;
#ifndef _arch_dreamcast
            if(smsvdp.borders)
                px = (smsvdp.framebuffer_base) + (line << smsvdp.fb_x);
            else
#endif /* !_arch_dreamcast */
                px = (smsvdp.framebuffer) + (line << smsvdp.fb_x);


            bg_draw(line, px);
            spr_draw(line, px);
        }
        else {
            /* Backgrounds can't actually affect anything status-wise, so there
               isn't any reason to do anything with them at all when skipping
               the frame. */
            spr_skip(line);
        }
    }
    else if(line < 192 && !skip) {
        pixel_t *px = (smsvdp.framebuffer_base) + (line << smsvdp.fb_x);

        /* Blank the whole scanline. */
        int tmp = (smsvdp.regs[7] & 0x0F);
        pixel_t col = tms9918_pal[tmp];

        for(i = 0; i < 256; ++i) {
            *px++ = col;
        }
    }

    if(line == 192) {
        /* Set the vblank flag, and signal an NMI, if desired */
        smsvdp.status |= 0x80;

        if(smsvdp.regs[1] & 0x20) {
            irqfunc();
        }
    }

    smsvdp.line = line;

    return 0;
}

void sms_vdp_data_write(uint8 data) {
    /* Clear the bytes written flag */
    smsvdp.flags &= (~SMS_VDP_FLAG_BYTES_WRITTEN);

    /* First, update the read buffer */
    smsvdp.read_buf = data;

    /* Check the destination of this byte */
    switch(smsvdp.code) {
        case 0x00:
        case 0x01:
        case 0x02:
            if(smsvdp.vram[smsvdp.addr] != data) {
                smsvdp.vram[smsvdp.addr] = data;
                smsvdp.pattern[smsvdp.addr >> 5].dirty = 1;
            }
            break;
        case 0x03:
            if(sms_cons._base.console_type != CONSOLE_GG) {
                smsvdp.cram[smsvdp.addr & 0x1F] = data;
                update_local_pal_sms(smsvdp.addr & 0x1F);
            }
            else {
                if(!(smsvdp.addr & 0x01)) {
                    smsvdp.pal_latch = data;
                }
                else {
                    smsvdp.cram[smsvdp.addr & 0x3E] = smsvdp.pal_latch;
                    smsvdp.cram[smsvdp.addr & 0x3F] = data;
                    update_local_pal_gg(smsvdp.addr & 0x3E);
                }
            }
            break;
    }

    /* Update the address register, and wrap, if needed */
    smsvdp.addr = (smsvdp.addr + 1) & 0x3FFF;
}

static void sms_vdp_reg_write(int reg, uint8 data) {
    smsvdp.regs[reg] = data;

    if(reg == 0) {
        sms_vdp_set_vidmode(smsvdp.vidmode, smsvdp.machine);

        if((smsvdp.regs[0] & 0x10) &&
           (smsvdp.flags & SMS_VDP_FLAG_LINE_INT)) {
            sms_z80_assert_irq();
        }
        else if((smsvdp.flags & SMS_VDP_FLAG_LINE_INT) &&
                !(smsvdp.status & 0x80)) {
            sms_z80_clear_irq();
        }
    }
    else if(reg == 1) {
        sms_vdp_set_vidmode(smsvdp.vidmode, smsvdp.machine);

        if((smsvdp.status & 0x80) && (smsvdp.regs[1] & 0x20)) {
            sms_z80_assert_irq();
        }
        else if((smsvdp.status & 0x80) &&
                !(smsvdp.flags & SMS_VDP_FLAG_LINE_INT)) {
            /* Don't clear the IRQ line if there is a line interrupt
               still going */
            sms_z80_clear_irq();
        }
    }
    else if(reg == 2) {
        readjust_name_table();
    }
    else if(reg == 5) {
        smsvdp.sat = smsvdp.vram + ((smsvdp.regs[5] & 0x7E) << 7);
    }
    else if(reg == 8) {
        int tmp = (smsvdp.regs[8] & 0xF8) >> 3;
        smsvdp.xscroll_coarse = (32 - tmp) & 0x1F;
        smsvdp.xscroll_fine = smsvdp.regs[8] & 0x07;
    }
    else if(reg == 9) {
        smsvdp.yscroll_fine = smsvdp.regs[9] & 0x07;
    }
}

void sms_vdp_ctl_write(uint8 data) {
    /* First, update the code/address registers */
    if(!(smsvdp.flags & SMS_VDP_FLAG_BYTES_WRITTEN)) {
        /* A multiple of 2 bytes written */
        smsvdp.addr = (smsvdp.addr & 0x3F00) | (data);
        smsvdp.addr_latch = data;
        smsvdp.flags |= SMS_VDP_FLAG_BYTES_WRITTEN;

        return;
    }

    /* One byte written, write the last one */
    smsvdp.addr = (data & 0x3F) << 8 | (smsvdp.addr_latch & 0xFF);
    smsvdp.code = (data & 0xC0) >> 6;
    smsvdp.flags &= (~SMS_VDP_FLAG_BYTES_WRITTEN);

    /* Code 0 - Read a byte of vram, put it in the buffer, and increment
        the address (that last part is important!) */
    if(smsvdp.code == 0x00) {
        smsvdp.read_buf = smsvdp.vram[smsvdp.addr];
        smsvdp.addr = (smsvdp.addr + 1) & 0x3FFF;
    }
    /* Code 2 - Register Write */
    else if(smsvdp.code == 0x02) {
        int reg = data & 0x0F;

        if(reg == 0x09 && smsvdp.line < smsvdp.lines) {
            smsvdp.v_scroll = (smsvdp.addr & 0xFF);
            smsvdp.flags |= SMS_VDP_FLAG_VSCROLL_CHG;
        }
        else {
            sms_vdp_reg_write(reg, smsvdp.addr & 0xFF);
        }
    }
}

void *sms_vdp_framebuffer(void) {
    return smsvdp.framebuffer;
}

void sms_vdp_framesize(uint32_t *x, uint32_t *y) {
    *x = 1 << smsvdp.fb_x;
    *y = 1 << smsvdp.fb_y;
}

void sms_vdp_activeframe(uint32_t *x, uint32_t *y, uint32_t *w, uint32_t *h) {
    if(sms_cons._base.console_type == CONSOLE_GG) {
        *x = 48;
        *y = 24;
        *w = 160;
        *h = 144;
    }
    else {
        if(!smsvdp.borders) {
            *x = *y = 0;
            *w = 256;
            *h = smsvdp.lines;
        }
        else {
            *x = *y = 0;
            *w = 284;

            if(smsvdp.vidmode == SMS_VIDEO_NTSC)
                *h = 243;
            else
                *h = 288;
        }
    }
}

uint8 sms_vdp_vcnt_read(void) {
    return vcnt_tab[smsvdp.line];
}

uint8 sms_vdp_hcnt_read(void) {
    return smsvdp.hcnt;
}

uint8 sms_vdp_data_read(void) {
    uint8 tmp;

    /* Clear the bytes written flag */
    smsvdp.flags &= (~SMS_VDP_FLAG_BYTES_WRITTEN);

    /* Fetch what is already in the buffer */
    tmp = smsvdp.read_buf;

    /* Fetch a new byte for the buffer */
    smsvdp.read_buf = smsvdp.vram[smsvdp.addr];

    /* And finally, increment the address */
    smsvdp.addr = (smsvdp.addr + 1) & 0x3FFF;

    return tmp;
}

uint8 sms_vdp_status_read(void) {
    uint8 tmp;

    /* Check the interrupt flags */
    if((smsvdp.status & 0x80) || (smsvdp.flags & SMS_VDP_FLAG_LINE_INT)) {
        sms_z80_clear_irq();
    }

    /* Clear the bytes written flag and line interrupt pending flag */
    smsvdp.flags &= (~(SMS_VDP_FLAG_BYTES_WRITTEN | SMS_VDP_FLAG_LINE_INT));

    /* Fetch our flags, so that we can clear stale ones */
    tmp = smsvdp.status;
    smsvdp.status &= (~0xE0);

    return tmp;
}

void sms_vdp_hcnt_latch(int cycles) {
    smsvdp.hcnt = sms_hcnt[cycles];
}

int sms_vdp_init(int mode, int borders) {
    int i, tmp;

#ifdef _arch_dreamcast
    (void)borders;
#endif

    /* Initialize the VDP emulation to a sane state */
    smsvdp.code = 0;
    smsvdp.addr = 0;
    smsvdp.read_buf = 0;
    smsvdp.status = 0;
    smsvdp.flags = 0;
    smsvdp.linecnt = 0xFF;
    smsvdp.pal_latch = 0;
    smsvdp.hcnt = 0;
    smsvdp.borders = 0;

    /* Set some sane register values */
    smsvdp.regs[0x0] = 0x04;
    smsvdp.regs[0x1] = 0x00;
    smsvdp.regs[0x2] = 0x0E;
    smsvdp.regs[0x3] = 0x00;
    smsvdp.regs[0x4] = 0x00;
    smsvdp.regs[0x5] = 0x7F;
    smsvdp.regs[0x6] = 0x00;
    smsvdp.regs[0x7] = 0x00;
    smsvdp.regs[0x8] = 0x00;
    smsvdp.regs[0x9] = 0x00;
    smsvdp.regs[0xA] = 0x00;

    tmp = (smsvdp.regs[8] & 0xF8) >> 3;
    smsvdp.xscroll_coarse = (32 - tmp) & 0x1F;
    smsvdp.xscroll_fine = smsvdp.regs[8] & 0x07;
    smsvdp.yscroll_fine = smsvdp.regs[9] & 0x07;

    /* Mark all patterns as dirty */
    for(i = 0; i < 512; ++i) {
        smsvdp.pattern[i].dirty = 1;
        /* There's no use in clearing the textures, since the will be
           overwritten before they're used, anyway */
    }

    /* Allocate memory */
    smsvdp.cram = (uint8 *)malloc(64);

    if(smsvdp.cram == NULL) {
#ifdef DEBUG
        fprintf(stderr, "sms_vdp_init: Could not allocate CRAM!\n");
#endif
        return -1;
    }

    smsvdp.pal = (pixel_t *)malloc(sizeof(pixel_t) * 32);

    if(smsvdp.pal == NULL) {
#ifdef DEBUG
        fprintf(stderr, "sms_vdp_init: Could not allocate local Palette!\n");
#endif
        free(smsvdp.cram);
        return -1;
    }

    smsvdp.vram = (uint8 *)malloc(0x4000);

    if(smsvdp.vram == NULL) {
#ifdef DEBUG
        fprintf(stderr, "sms_vdp_init: Could not allocate VRAM!\n");
#endif
        free(smsvdp.cram);
        free(smsvdp.pal);
        return -1;
    }

#ifndef _arch_dreamcast
    if(!borders) {
        smsvdp.framebuffer = (pixel_t *)malloc(256 * 256 * sizeof(pixel_t));
#else
        smsvdp.framebuffer =
            (pixel_t *)memalign(32, 256 * 256 * sizeof(pixel_t));
#endif
        smsvdp.fb_x = 8;
        smsvdp.fb_y = 8;
        smsvdp.borders = 0;
#ifndef _arch_dreamcast
    }
    else {
        smsvdp.framebuffer = (pixel_t *)malloc(512 * 512 * sizeof(pixel_t));

        smsvdp.fb_x = 9;
        smsvdp.fb_y = 9;
        smsvdp.borders = 1;
    }
#endif

    if(smsvdp.framebuffer == NULL) {
#ifdef DEBUG
        fprintf(stderr, "sms_vdp_init: Could not allocate framebuffer!\n");
#endif
        free(smsvdp.cram);
        free(smsvdp.pal);
        free(smsvdp.vram);
        return -1;
    }

#ifdef _arch_dreamcast
    /* Put the framebuffer in P2 so that it is not cacheable. */
    smsvdp.framebuffer = (pixel_t *)((uint32)smsvdp.framebuffer | 0xA0000000);
#endif

    sms_vdp_set_vidmode(mode, SMS_VDP_MACHINE_SMS2);

    for(i = 0; i < 256; ++i) {
        lut[i] = (i & 0x01) | ((i & 0x02) << 3) | ((i & 0x04) << 6) |
            ((i & 0x08) << 9) | ((i & 0x10) << 12) | ((i & 0x20) << 15) |
            ((i & 0x40) << 18) | ((i & 0x80) << 21);
    }

    readjust_name_table();
    smsvdp.sat = smsvdp.vram + ((smsvdp.regs[5] & 0x7E) << 7);

    return 0;
}

int sms_vdp_reset(void) {
    int i, tmp;

    /* Initialize the VDP emulation to a sane state */
    smsvdp.code = 0;
    smsvdp.addr = 0;
    smsvdp.read_buf = 0;
    smsvdp.status = 0;
    smsvdp.flags = 0;
    smsvdp.linecnt = 0xFF;
    smsvdp.pal_latch = 0;
    smsvdp.hcnt = 0;

    /* Set some sane register values */
    smsvdp.regs[0x0] = 0x04;
    smsvdp.regs[0x1] = 0x00;
    smsvdp.regs[0x2] = 0x0E;
    smsvdp.regs[0x3] = 0x00;
    smsvdp.regs[0x4] = 0x00;
    smsvdp.regs[0x5] = 0x7F;
    smsvdp.regs[0x6] = 0x00;
    smsvdp.regs[0x7] = 0x00;
    smsvdp.regs[0x8] = 0x00;
    smsvdp.regs[0x9] = 0x00;
    smsvdp.regs[0xA] = 0x00;

    tmp = (smsvdp.regs[8] & 0xF8) >> 3;
    smsvdp.xscroll_coarse = (32 - tmp) & 0x1F;
    smsvdp.xscroll_fine = smsvdp.regs[8] & 0x07;
    smsvdp.yscroll_fine = smsvdp.regs[9] & 0x07;

    /* Mark all patterns as dirty */
    for(i = 0; i < 512; ++i) {
        smsvdp.pattern[i].dirty = 1;
        /* There's no use in clearing the textures, since the will be
           overwritten before they're used, anyway */
    }

    memset(smsvdp.vram, 0, 0x4000);
    memset(smsvdp.cram, 0, 64);
    memset(smsvdp.pal, 0, sizeof(pixel_t) * 32);

    sms_vdp_set_vidmode(smsvdp.vidmode, smsvdp.machine);

    return 0;
}

int sms_vdp_shutdown(void) {
#ifdef _arch_dreamcast
    /* Put the framebuffer back into P1. */
    smsvdp.framebuffer = (pixel_t *)((uint32)smsvdp.framebuffer & 0xDFFFFFFF);
#endif

    /* Free all RAM used */
    free(smsvdp.cram);
    free(smsvdp.pal);
    free(smsvdp.vram);
    free(smsvdp.framebuffer);

    return 0;
}

void sms_vdp_set_vidmode(int mode, int machine) {
    int oldlines = smsvdp.lines;
    int vdp_mode;

    vdp_mode = ((smsvdp.regs[0x0] & 0x04) << 1) |
        ((smsvdp.regs[0x1] & 0x08) >> 1) |
        ((smsvdp.regs[0x0] & 0x02)) |
        ((smsvdp.regs[0x1] & 0x10) >> 4);

    switch(vdp_mode) {
        case 0x00: /* Mode 0: TMS9918 Graphic 1 Mode */
            bg_draw = &tms9918a_m0_draw_bg;
            spr_draw = &tms9918a_m023_draw_spr;
            spr_skip = &tms9918a_m023_skip_spr;
            break;

        case 0x01: /* Mode 1: TMS9918 Text mode */
            bg_draw = &tms9918a_m1_draw_bg;
            /* The sprite system is inactive */
            spr_draw = &dummy_draw;
            spr_skip = &dummy_skip;
            break;

        case 0x02: /* Mode 2: TMS9918 Graphic 2 Mode */
            bg_draw = &tms9918a_m2_draw_bg;
            spr_draw = &tms9918a_m023_draw_spr;
            spr_skip = &tms9918a_m023_skip_spr;
            break;

        case 0x04: /* Mode 3: TMS9918 Multicolor mode */
            bg_draw = &tms9918a_m3_draw_bg;
            spr_draw = &tms9918a_m023_draw_spr;
            spr_skip = &tms9918a_m023_skip_spr;
            break;

        case 0x08: /* Mode 4 */
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0E:
        case 0x0F:
            if(machine != SMS_VDP_MACHINE_TMS9918A) {
                bg_draw = &sms_vdp_m4_draw_bg;
                spr_draw = &sms_vdp_m4_draw_spr;
                spr_skip = &sms_vdp_m4_skip_spr;
            }
            else {
#ifdef DEBUG
                fprintf(stderr, "sms_vdp_set_vidmode: Unsupported mode for "
                        "TMS9918A VDP: 0x%02x.\n", vdp_mode);
                return;
#endif
            }
            break;

        default:
#ifdef DEBUG
            fprintf(stderr, "sms_vdp_set_vidmode: Unsupported mode 0x%02x.\n",
                    vdp_mode);
#endif
            break;
    }

    if(mode == SMS_VIDEO_NTSC) {
        smsvdp.vidmode = SMS_VIDEO_NTSC;

        if(machine == SMS_VDP_MACHINE_SMS2) {
            smsvdp.machine = SMS_VDP_MACHINE_SMS2;

            /* Check the M1, M2, M3, and M4 bits for the video mode */
            if((smsvdp.regs[0] & 0x06) == 0x06) {

                /* M4 and M2 are set: M1/M3 determine resolution */
                if(!(smsvdp.regs[1] & 0x18) ||
                   (smsvdp.regs[1] & 0x18) == 0x18) {
                    /* Either both of M1/M3 are set, or neither is set:
                       192 line mode */
                    vcnt_tab = vcnt_ntsc_192;
                    smsvdp.lines = 192;
                    smsvdp.framebuffer_base =
                        smsvdp.framebuffer + (top_border[0] << smsvdp.fb_x);
                }
                else if(smsvdp.regs[1] & 0x10) {
                    /* M1 is set: 224 line mode */
                    vcnt_tab = vcnt_ntsc_224;
                    smsvdp.lines = 224;
                    smsvdp.framebuffer_base =
                        smsvdp.framebuffer + (top_border[1] << smsvdp.fb_x);
                }
                else if(smsvdp.regs[1] & 0x08) {
                    /* M3 is set: 240 line mode */
                    vcnt_tab = vcnt_ntsc_240;
                    smsvdp.lines = 240;
                    smsvdp.framebuffer_base =
                        smsvdp.framebuffer + (top_border[2] << smsvdp.fb_x);
                }
                else {
                    /* Invalid text mode.... */
                }
            }
            else if(smsvdp.regs[0] & 0x04) {
                /* M4 is set */
                if(smsvdp.regs[1] & 0x10) {
                    /* M1 is set: Invalid text mode */
                }
                else {
                    /* M1 is not set: 192 line mode */
                    vcnt_tab = vcnt_ntsc_192;
                    smsvdp.lines = 192;
                    smsvdp.framebuffer_base =
                        smsvdp.framebuffer + (top_border[0] << smsvdp.fb_x);
                }
            }
            else {
                /* M4 is not set, use TMS9918 modes */
                vcnt_tab = vcnt_ntsc_192;
                smsvdp.lines = 192;
                smsvdp.framebuffer_base =
                    smsvdp.framebuffer + (top_border[0] << smsvdp.fb_x);
            }

            if(sms_cons._base.console_type != CONSOLE_GG) {
                if(smsvdp.lines == 192) {
                    gui_set_aspect(4.0f, 3.0f);
                }
                else if(smsvdp.lines == 224) {
                    gui_set_aspect(8.0f, 7.0f);
                }
                else {
                    gui_set_aspect(16.0f, 15.0f);
                }
            }
            else {
                gui_set_aspect(10.0f, 9.0f);
            }
        }
        else if(machine == SMS_VDP_MACHINE_SMS1) {
        }
        else if(machine == SMS_VDP_MACHINE_TMS9918A) {
            vcnt_tab = vcnt_ntsc_192;
            smsvdp.lines = 192;
            smsvdp.framebuffer_base =
                smsvdp.framebuffer + (top_border[0] << smsvdp.fb_x);
            gui_set_aspect(4.0f, 3.0f);
        }
        else {
#ifdef DEBUG
            fprintf(stderr, "sms_vdp_set_vidmode: Invalid machine: %d\n",
                    machine);
#endif
        }
    }
    else if(mode == SMS_VIDEO_PAL) {
        smsvdp.vidmode = SMS_VIDEO_PAL;

        if(machine == SMS_VDP_MACHINE_SMS2) {
            smsvdp.machine = SMS_VDP_MACHINE_SMS2;

            /* Check the M1, M2, M3, and M4 bits for the video mode */
            if((smsvdp.regs[0] & 0x06) == 0x06) {
                
                /* M4 and M2 are set: M1/M3 determine resolution */
                if(!(smsvdp.regs[1] & 0x18) ||
                   (smsvdp.regs[1] & 0x18) == 0x18) {
                    /* Either both of M1/M3 are set, or neither is set:
                       192 line mode */
                    vcnt_tab = vcnt_pal_192;
                    smsvdp.lines = 192;
                    smsvdp.framebuffer_base =
                        smsvdp.framebuffer + (top_border[3] << smsvdp.fb_x);
                }
                else if(smsvdp.regs[1] & 0x10) {
                    /* M1 is set: 224 line mode */
                    vcnt_tab = vcnt_pal_224;
                    smsvdp.lines = 224;
                    smsvdp.framebuffer_base =
                        smsvdp.framebuffer + (top_border[4] << smsvdp.fb_x);
                }
                else if(smsvdp.regs[1] & 0x08) {
                    /* M3 is set: 240 line mode */
                    vcnt_tab = vcnt_pal_240;
                    smsvdp.lines = 240;
                    smsvdp.framebuffer_base =
                        smsvdp.framebuffer + (top_border[5] << smsvdp.fb_x);
                }
                else {
                    /* Invalid text mode.... */
                }
            }
            else if(smsvdp.regs[0] & 0x04) {
                /* M4 is set */
                if(smsvdp.regs[1] & 0x10) {
                    /* M1 is set: Invalid text mode */
                }
                else {
                    /* M1 is not set: 192 line mode */
                    vcnt_tab = vcnt_pal_192;
                    smsvdp.lines = 192;
                    smsvdp.framebuffer_base =
                        smsvdp.framebuffer + (top_border[3] << smsvdp.fb_x);
                }
            }
            else {
                /* M4 is not set, use TMS9918 modes */
                vcnt_tab = vcnt_pal_192;
                smsvdp.lines = 192;
                smsvdp.framebuffer_base =
                    smsvdp.framebuffer + (top_border[3] << smsvdp.fb_x);
            }

            if(sms_cons._base.console_type != CONSOLE_GG) {
                if(smsvdp.lines == 192) {
                    gui_set_aspect(4.0f, 3.0f);
                }
                else if(smsvdp.lines == 224) {
                    gui_set_aspect(8.0f, 7.0f);
                }
                else {
                    gui_set_aspect(16.0f, 15.0f);
                }
            }
            else {
                gui_set_aspect(10.0f, 9.0f);
            }
        }
        else if(machine == SMS_VDP_MACHINE_SMS1) {
        }
        else if(machine == SMS_VDP_MACHINE_TMS9918A) {
            vcnt_tab = vcnt_pal_192;
            smsvdp.lines = 192;
            smsvdp.framebuffer_base =
                smsvdp.framebuffer + (top_border[0] << smsvdp.fb_x);
            gui_set_aspect(4.0f, 3.0f);
        }
        else {
#ifdef DEBUG
            fprintf(stderr, "sms_vdp_set_vidmode: Invalid machine: %d\n",
                    machine);
#endif
        }
    }
    else {
#ifdef DEBUG
        fprintf(stderr, "sms_vdp_set_vidmode: Invalid mode %d\n", mode);
#endif
    }

    if(oldlines != smsvdp.lines) {
        readjust_name_table();
    }
}

static int sms_vdp_write_vram_context(FILE *fp) {
    uint8 data[4];

    data[0] = 'V';
    data[1] = 'R';
    data[2] = 'A';
    data[3] = 'M';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(0x4010, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(smsvdp.vram, 1, 0x4000, fp);
    return 0;
}

static int sms_vdp_read_vram_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 0x4010)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    memset(smsvdp.vram, 0, 0x4000);
    memcpy(smsvdp.vram, buf + 16, 0x4000);
    return 0;
}

static int sms_vdp_write_cram_context(FILE *fp) {
    uint8 data[4];

    /* Don't write anything for SG-1000, SC-3000, or ColecoVision. */
    if(sms_cons._base.console_type > CONSOLE_GG)
        return 0;

    data[0] = 'C';
    data[1] = 'R';
    data[2] = 'A';
    data[3] = 'M';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(80, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(smsvdp.cram, 1, 64, fp);
    return 0;
}

static int sms_vdp_read_cram_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 80)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    memset(smsvdp.cram, 0, 64);
    memcpy(smsvdp.cram, buf + 16, 64);
    return 0;
}

int sms_vdp_write_context(FILE *fp) {
    uint8 data[4];
    uint32 len;

    data[0] = '9';
    data[1] = '9';
    data[2] = '1';
    data[3] = '8';
    fwrite(data, 1, 4, fp);             /* Block ID */

    if(sms_cons._base.console_type < CONSOLE_SG1000)
        len = 0x4010 + 80 + 48;         /* VRAM + CRAM + this block */
    else
        len = 0x4010 + 48;              /* VRAM + this block */

    UINT32_TO_BUF(len, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    UINT32_TO_BUF(32, data);
    fwrite(data, 1, 4, fp);             /* Child pointer */

    fwrite(smsvdp.regs, 1, 16, fp);

    UINT16_TO_BUF(smsvdp.addr, data);
    fwrite(data, 1, 2, fp);

    fwrite(&smsvdp.read_buf, 1, 1, fp);
    fwrite(&smsvdp.status, 1, 1, fp);
    fwrite(&smsvdp.code, 1, 1, fp);
    fwrite(&smsvdp.addr_latch, 1, 1, fp);
    fwrite(&smsvdp.linecnt, 1, 1, fp);
    fwrite(&smsvdp.pal_latch, 1, 1, fp);
    fwrite(&smsvdp.hcnt, 1, 1, fp);

    data[0] = data[1] = data[2] = 0;
    fwrite(data, 1, 3, fp);

    UINT32_TO_BUF(smsvdp.flags, data);
    fwrite(data, 1, 4, fp);

    if(sms_vdp_write_vram_context(fp)) {
        return -1;
    }
    else if(sms_vdp_write_cram_context(fp)) {
        return -1;
    }

    return 0;
}

int sms_vdp_read_context(const uint8 *buf) {
    uint32 len, child, clen;
    uint16 ver;
    const uint8 *ptr;
    int rv, i, tmp;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len < 48)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    BUF_TO_UINT32(buf + 12, child);
    if(child != 32)
        return -1;

    /* Read the VDP state */
    for(i = 0; i < 16; ++i) {
        smsvdp.regs[i] = buf[16 + i];
    }

    BUF_TO_UINT16(buf + 32, smsvdp.addr);
    smsvdp.read_buf = buf[34];
    smsvdp.status = buf[35];
    smsvdp.code = buf[36];
    smsvdp.addr_latch = buf[37];
    smsvdp.linecnt = buf[38];
    smsvdp.pal_latch = buf[39];
    smsvdp.hcnt = buf[40];
    BUF_TO_UINT32(buf + 44, smsvdp.flags);

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
            case FOURCC_TO_UINT32('V', 'R', 'A', 'M'):
                rv = sms_vdp_read_vram_context(ptr);
                break;

            case FOURCC_TO_UINT32('C', 'R', 'A', 'M'):
                rv = sms_vdp_read_cram_context(ptr);
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

    if(sms_cons._base.console_type != CONSOLE_GG) {
        for(i = 0; i < 0x20; ++i) {
            update_local_pal_sms(i);
        }
    }
    else {
        for(i = 0; i < 0x40; i += 2) {
            update_local_pal_gg(i);
        }
    }

    /* Deal with the updated register values... */
    sms_z80_clear_irq();
    smsvdp.sat = smsvdp.vram + ((smsvdp.regs[5] & 0x7E) << 7);
    tmp = (smsvdp.regs[8] & 0xF8) >> 3;
    smsvdp.xscroll_coarse = (32 - tmp) & 0x1F;
    smsvdp.xscroll_fine = smsvdp.regs[8] & 0x07;
    smsvdp.yscroll_fine = smsvdp.regs[9] & 0x07;
    sms_vdp_set_vidmode(smsvdp.vidmode, smsvdp.machine);
    readjust_name_table();

    /* Assert the irq line if we need to. */
    if((smsvdp.status & 0x80) && (smsvdp.regs[1] & 0x20)) {
        sms_z80_assert_irq();
    }

    /* Mark all patterns as dirty */
    for(i = 0; i < 512; ++i) {
        smsvdp.pattern[i].dirty = 1;
        /* There's no use in clearing the textures, since the will be
           overwritten before they're used, anyway */
    }

    return 0;
}

void sms_vdp_read_context_v1(FILE *fp) {
    uint8 byte[4];
    int i;

    fread(&smsvdp.code, 1, 1, fp);

    fread(&byte, 2, 1, fp);
    smsvdp.addr = byte[0] | (byte[1] << 8);

    fread(&smsvdp.addr_latch, 1, 1, fp);
    fread(&smsvdp.read_buf, 1, 1, fp);
    fread(smsvdp.cram, 64, 1, fp);
    fread(&smsvdp.status, 1, 1, fp);
    fread(smsvdp.regs, 16, 1, fp);
    fread(smsvdp.vram, 0x4000, 1, fp);
    fread(&smsvdp.hcnt, 1, 1, fp);
    fread(&smsvdp.pal_latch, 1, 1, fp);
    fread(&smsvdp.linecnt, 1, 1, fp);

    fread(&byte, 4, 1, fp);
    smsvdp.flags = byte[0] | (byte[1] << 8) | (byte[2] << 16) | (byte[3] << 24);

    /* Mark all patterns as dirty */
    for(i = 0; i < 512; ++i) {
        smsvdp.pattern[i].dirty = 1;
        /* There's no use in clearing the textures, since the will be
           overwritten before they're used, anyway */
    }

    if(sms_cons._base.console_type != CONSOLE_GG) {
        for(i = 0; i < 0x20; ++i) {
            update_local_pal_sms(i);
        }
    }
    else {
        for(i = 0; i < 0x40; i += 2) {
            update_local_pal_gg(i);
        }
    }
}
