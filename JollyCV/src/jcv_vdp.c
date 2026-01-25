/*
Copyright (c) 2020-2022 Rupert Carmichael
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// ColecoVision VDP - Texas Instruments TMS9928A

#include <stdint.h>
#include <string.h>

#include "jcv_serial.h"
#include "jcv_vdp.h"
#include "jcv_z80.h"

// The Carmichael Experience - Tweaked to Look Nice
static const uint32_t palette_teatime[16] = {
    0xff000000, 0xff000000, 0xff23b03f, 0xff3cdf5e,
    0xff495bfe, 0xff757cff, 0xffd73218, 0xff14f8f8,
    0xffff4746, 0xffff6464, 0xffd4ce54, 0xffe6e180,
    0xff1d9a34, 0xffd63bc1, 0xffcccccc, 0xffffffff,
};

// Based on tms9918a.txt by Sean Young (the one most other emulators use)
static const uint32_t palette_syoung[16] = {
    0xff000000, 0xff000000, 0xff21c842, 0xff5edc78,
    0xff5455ed, 0xff7d76fc, 0xffd4524d, 0xff42ebf5,
    0xfffc5554, 0xffff7978, 0xffd4c154, 0xffe6ce80,
    0xff21b03b, 0xffc95bba, 0xffcccccc, 0xffffffff,
};

static const uint32_t *palette = palette_teatime;

static cv_vdp_t vdp; // VDP Context

static uint32_t *vbuf = NULL;

static uint16_t numscanlines = CV_VDP_SCANLINES;

// Increment address with wrap
static inline void jcv_vdp_addr_inc(void) {
    vdp.addr = (vdp.addr + 1) & 0x3fff;
}

// Test if rendering is enabled or disabled (BL bit)
static inline uint8_t jcv_vdp_rendering(void) {
    return vdp.ctrl[1] & 0x40;
}

// Test if the GINT bit is set in control register 1
static inline uint8_t jcv_vdp_gint(void) {
    return vdp.ctrl[1] & 0x20;
}

// Test if the INT bit is set in the status register
static inline uint8_t jcv_vdp_int(void) {
    return vdp.stat & 0x80;
}

// Retrieve the current backdrop colour
static inline uint32_t jcv_vdp_bdcol(void) {
    return palette[vdp.ctrl[7] & 0x0f];
}

// Draw a line of backdrop colour
static inline void jcv_vdp_bdline(int line) {
    for (int i = 0; i < CV_VDP_WIDTH_OVERSCAN; ++i)
        vbuf[(line * CV_VDP_WIDTH_OVERSCAN) + i] = jcv_vdp_bdcol();
}

// Draw a single pixel onto the canvas
static inline void jcv_vdp_pixel(uint32_t c, int line, int dot) {
    vbuf[((line + CV_VDP_OVERSCAN) * CV_VDP_WIDTH_OVERSCAN) + dot] = c;
}

// Set the video output buffer to be written to
void jcv_vdp_set_buffer(uint32_t *ptr) {
    vbuf = ptr;
}

// Set the video palette
void jcv_vdp_set_palette(uint8_t p) {
    switch (p) {
        case 0:
            palette = palette_teatime; break;
        case 1:
            palette = palette_syoung; break;
        default:
            break;
    }
}

// Set the region
void jcv_vdp_set_region(uint8_t region) {
    // 313 scanlines for PAL, 262 scanlines for NTSC (192 visible for both)
    numscanlines = region ? CV_VDP_SCANLINES_PAL : CV_VDP_SCANLINES;
}

// Set initial VDP values - also could be called reset
void jcv_vdp_init(void) {
    vdp.line = 0;
    vdp.dot = 0;

    // Set VDP control register defaults
    for (int i = 0; i < 8; ++i)
        vdp.ctrl[i] = 0x00;

    vdp.stat = 0x00; // Zero the Status register

    memset(vdp.vram, 0x00, SIZE_VRAM); // Zero the VRAM

    // Zero the latches and address register
    vdp.addr = 0x0000;
    vdp.dlatch = 0x00;
    vdp.wlatch = 0x00;

    vdp.tbl_col = vdp.ctrl[3] << 6;
    vdp.tbl_pname = vdp.ctrl[2] << 10;
    vdp.tbl_pgen = vdp.ctrl[4] << 11;
    vdp.tbl_sattr = vdp.ctrl[5] << 7;
    vdp.tbl_spgen = vdp.ctrl[6] << 11;

}

uint8_t jcv_vdp_rd_data(void) {
    vdp.wlatch = 0; // Make sure the write latch is clear
    uint8_t rb = vdp.dlatch; // Store original latch value
    vdp.dlatch = vdp.vram[vdp.addr]; // Read new data into the latch
    jcv_vdp_addr_inc(); // Increment address
    return rb; // Return the value before the read-ahead
}

uint8_t jcv_vdp_rd_stat(void) {
    vdp.wlatch = 0; // Make sure the write latch is clear
    uint8_t sr = vdp.stat; // Store original register value for return
    vdp.stat &= 0x1f; // Clear INT, 5S, and C flags on this register
    return sr; // Return the old register value
}

static void jcv_vdp_wr_reg(uint8_t rnum, uint8_t data) {
    /*     |----------------------------------------------------------------|
       Bit |7       6       5       4       3       2       1       0       |
    Reg    |----------------------------------------------------------------|
    0      |-       -       -       -       -       -       M2      EXTVID  |
    1      |4/16K   BL      GINT    M1      M3      -       SI      MAG     |
    2      |-       -       -       -       PN13    PN12    PN11    PN10    |
    3      |CT13    CT12    CT11    CT10    CT9     CT8     CT7     CT6     |
    4      |-       -       -       -       -       PG13    PG12    PG11    |
    5      |-       SA13    SA12    SA11    SA10    SA9     SA8     SA7     |
    6      |-       -       -       -       -       SG13    SG12    SG11    |
    7      |TC3     TC2     TC1     TC0     BD3     BD2     BD1     BD0     |
           |----------------------------------------------------------------|
    */
    // Masks to avoid writing "Don't Care" bits
    uint8_t dcmask[8] = { 0x03, 0xfb, 0x0f, 0xff, 0x07, 0x7f, 0x07, 0xff };

    // Save the GINT bit status before writing to a register
    const uint8_t old_gint = jcv_vdp_gint();

    vdp.ctrl[rnum] = data & dcmask[rnum]; // Write to the register

    // Bit shifts in cases 2-6 create a 14-bit address offset from the
    // start of VRAM, based on the value written to the register
    switch (rnum) {
        case 0: { // Mode Control 1
            // Screen mode may have changed - handle in drawing routines
            break;
        }
        case 1: { // Mode Control 2
            // Screen mode may have changed - handle in drawing routines
            // Fire NMI if Status INT is set and Register 1 GINT bit was set
            if (jcv_vdp_int() && jcv_vdp_gint() && !old_gint)
                jcv_z80_nmi();
            break;
        }
        case 2: { // Pattern Name Table
            vdp.tbl_pname = vdp.ctrl[2] << 10;
            break;
        }
        case 3: { // Colour Table
            vdp.tbl_col = vdp.ctrl[3] << 6;
            break;
        }
        case 4: { // Pattern Generator Table
            vdp.tbl_pgen = vdp.ctrl[4] << 11;
            break;
        }
        case 5: { // Sprite Attribute Table
            vdp.tbl_sattr = vdp.ctrl[5] << 7;
            break;
        }
        case 6: { // Sprite Pattern Generator
            vdp.tbl_spgen = vdp.ctrl[6] << 11;
            break;
        }
        case 7: { // Foreground/Backdrop Colours
            // These are determined on the fly in the drawing routines
            break;
        }
        default:
            break;
    }
}

void jcv_vdp_wr_ctrl(uint8_t data) {
    if (vdp.wlatch) { // Second Write
        vdp.wlatch = 0; // Flip the latch back to indicate the write is done

        uint16_t upper = (data & 0x3f) << 8; // Upper address byte
        vdp.addr = upper | vdp.dlatch; // OR the full address together

        switch (data & 0xc0) { // Check if this is a register write or not
            case 0x00: { // Read VRAM data into the latch and increment address
                vdp.dlatch = vdp.vram[vdp.addr]; // Read data into data latch
                jcv_vdp_addr_inc(); // Increment address
                break;
            }
            case 0x80: { // Write the data latch value into the register
                jcv_vdp_wr_reg(data & 0x07, vdp.dlatch); // 3 bits for register
                break;
            }
            default:
                break;
        }
    }
    else { // First Write
        vdp.wlatch = 1; // Set the write latch to indicate one byte was written
        vdp.addr = (vdp.addr & 0x3f00) | data; // Write lower address byte
        vdp.dlatch = data; // Store the lower byte in the latch
    }
}

// Write data to the VDP
void jcv_vdp_wr_data(uint8_t data) {
    vdp.wlatch = 0; // Make sure the write latch is clear
    vdp.dlatch = vdp.vram[vdp.addr] = data; // Write data to the latch and VRAM
    jcv_vdp_addr_inc(); // Increment Address
}

// Draw a single line of background pixels
static void jcv_vdp_bgline(void) {
    uint32_t bg, fg; // Colour value of palette entries
    uint8_t pindex = 0; // Palette Index (upper 4 bits = fg, lower 4 bits = bg)
    uint8_t chpat = 0; // One row of pixel data (Character Pattern)

    uint8_t srow = vdp.line >> 3; // Screen row being drawn (0 to 23, 8 high)
    uint8_t prow = vdp.line & 0x07; // Pattern row being drawn (0 to 7)

    uint16_t offset_col; // Colour offset
    uint16_t offset_pgen; // Pattern Generator Table address offset
    uint16_t offset_pname; // Pattern Name Table address offset

    // Screen mode
    uint8_t scrmode = ((vdp.ctrl[1] & 0x10) >> 4) | // Bit 0 (M1)
        (vdp.ctrl[0] & 0x02) | // Bit 1 (M2)
        ((vdp.ctrl[1] & 0x08) >> 1); // Bit 2 (M3)

    /* Control Register 4, which sets the Pattern Generator address offset, has
       a special function in Mode 2. Only bit 2 (PG13) sets the address of the
       Pattern Generator, resulting in either 0x0000 or 0x2000. Shift PG13 left
       11 positions to create the 14-bit address offset.
    */
    offset_pgen = (vdp.ctrl[4] & 0x04) << 11;

    // Special case for Text Mode
    if (scrmode == 0x01) {
        /* VDP Control Register 7
          7  6  5  4   3  2  1  0
        ---------------------------
        | Foreground | Background | 4 bits represent the palette entry.
        ---------------------------
        */
        fg = palette[(vdp.ctrl[7] >> 4) & 0x0f];
        bg = jcv_vdp_bdcol();

        // Draw 16 pixel left/right borders in text mode, using backdrop colour
        for (int p = 0; p < CV_VDP_OVERSCAN << 1; ++p) {
            jcv_vdp_pixel(jcv_vdp_bdcol(), vdp.line, vdp.dot++);
            jcv_vdp_pixel(jcv_vdp_bdcol(), vdp.line, p + 256);
        }

        // The screen is divided into a grid of 40 text positions aross and 24
        // down. Each of the text positions is 6 pixels wide and 8 pixels high.
        for (int i = 0; i < 40; ++i) {
            offset_pname = vdp.vram[vdp.tbl_pname + (srow * 40) + i];
            pindex = vdp.vram[vdp.tbl_pgen + (offset_pname << 3) + prow];

            // In Text Mode, the least significant two pixels are ignored (6x8)
            // All set bits are foreground, unset bits are background
            for (int p = 0x80; p > 0x02; p >>= 1)
                jcv_vdp_pixel(pindex & p ? fg : bg, vdp.line, vdp.dot++);
        }

        vdp.dot = 0; // Reset the dot counter
        return; // Pixels for Text Mode are now drawn
    }

    // Draw left overscan
    for (int i = 0; i < CV_VDP_OVERSCAN; ++i)
        jcv_vdp_pixel(jcv_vdp_bdcol(), vdp.line, vdp.dot++);

    // Graphics 1/2 and Multicolor Modes - Info on shifts in Datasheet, 3-3
    for (int i = 0; i < 32; ++i) { // 256 pixels - 32 tiles, 8 pixels wide each
        if (scrmode == 0x00) { // Mode 0: Graphics 1
            offset_pname = vdp.vram[vdp.tbl_pname + (srow << 5) + i];
            chpat = vdp.vram[vdp.tbl_pgen + (offset_pname << 3) + prow];
            pindex = vdp.vram[vdp.tbl_col + (offset_pname >> 3)];
        }
        else if (scrmode == 0x02) { // Mode 2: Graphics 2
            // In mode 2, offset is incremented by 0, 0x100, and 0x200 for each
            // 1/3 of the screen. Top = 0, Middle = 0x100, Bottom = 0x200
            offset_pname = vdp.vram[vdp.tbl_pname + (srow << 5) + i];
            offset_pname += (srow & 0x18) << 5; // Increment if required
            offset_col = vdp.tbl_col & 0x2000;

            /* Control Register 4 bits 0 and 1 are an AND mask over the
               character number. The character number is 0 - 767 (0x2ff) and
               these two bits are ANDed over the two highest bits of this value
               (0x2ff is 10 bits, so bit 8 and 9). So in effect, if bit 0 of
               Control Register 4 is set, the second array of 256 patterns in
               the Pattern Generator table is used for the middle 8 rows
               of characters, otherwise the first 256 patterns. If bit 1 is set,
               the third chunk of patterns is used in the Pattern Generator,
               otherwise the first. OR 0xff to fill in the zeros from the shift
               operation.
            */
            uint16_t m1 = ((vdp.ctrl[4] & 0x03) << 8) | 0xff;

            /* Control Register 3 has a different meaning. Only bit 7 (CT13)
               sets the Colour Table address. Somewhat like Control Register 4
               for the Pattern Generator, bits 6 - 0 are an AND mask over the
               top 7 bits of the character number. OR 0x07 to fill in the zeros
               from the shift operation.
            */
            uint16_t m2 = ((vdp.ctrl[3] & 0x7f) << 3) | 0x07;

            // Use the masks here to select the proper pattern/colour offsets
            chpat = vdp.vram[offset_pgen + ((offset_pname & m1) << 3) + prow];
            pindex = vdp.vram[offset_col + ((offset_pname & m2) << 3) + prow];
        }
        else if (scrmode == 0x04) { // Mode 3: Multicolor
            /* 2 bytes from the Pattern Generator table represent four colours.
            The address for the first byte can be calculated as follows:
            PG + (byte in PN) x 8 + (row AND 3) x 2
            Simply increment the address by one for the second byte.

            8x8 colour block made up of 4 4x4 blocks
            -----------------------------------------
            |   7   6   5   4   |   3   2   1   0   |   One byte represents 8
            -----------------------------------------   pixels, with the 4 most
            |     Colour A      |     Colour B      |   significant bits for the
            |  PG Byte 0 >> 4   |  PG Byte 0 & 0xf  |   left palette entry, and
            -----------------------------------------   4 least significant bits
            |       Colour C    |       Colour D    |   for the right entry.
            |  PG Byte 1 >> 4   |  PG Byte 1 & 0xf  |
            -----------------------------------------
            */
            offset_pname = vdp.vram[vdp.tbl_pname + (srow << 5) + i];

            // Address of the colour offset, incremented by 1 for bottom 4 rows
            offset_col = offset_pgen + (offset_pname << 3) +
                ((srow & 0x03) << 1) + (vdp.line & 0x04 ? 1 : 0);

            pindex = vdp.vram[offset_col]; // Palette index

            // fg for left, bg for right - reusing variables for convenience
            fg = pindex >> 4 ? palette[pindex >> 4] : jcv_vdp_bdcol();
            bg = pindex & 0x0f ? palette[pindex & 0x0f] : jcv_vdp_bdcol();

            // Draw left and right background data
            for (int p = 0; p < 4; ++p)
                jcv_vdp_pixel(fg, vdp.line, vdp.dot++);

            for (int p = 0; p < 4; ++p)
                jcv_vdp_pixel(bg, vdp.line, vdp.dot++);

            continue; // Pixels are already drawn, skip the rest of the loop
        }

        // Set foreground and background values, if 0 use the backdrop colour
        bg = pindex & 0x0f ? palette[pindex & 0x0f] : jcv_vdp_bdcol();
        fg = pindex >> 4 ? palette[pindex >> 4] : jcv_vdp_bdcol();

        // Draw pattern data starting from the leftmost pixel
        for (int p = 0x80; p > 0x00; p >>= 1)
            jcv_vdp_pixel(chpat & p ? fg : bg, vdp.line, vdp.dot++);
    }

    // Draw right overscan
    for (int i = 0; i < CV_VDP_OVERSCAN; ++i)
        jcv_vdp_pixel(jcv_vdp_bdcol(), vdp.line, vdp.dot++);

    vdp.dot = 0; // Reset the dot counter
}

// Draw a single line of sprite pixels
static void jcv_vdp_sprline(void) {
    uint8_t sprmag = vdp.ctrl[1] & 0x01; // Sprites are magnified (doubled)
    uint8_t sprsize = vdp.ctrl[1] & 0x02 ? 16 : 8; // 16x16 if SI bit set

    uint8_t numspr = 0;

    // Buffer palette entry data for this line
    uint8_t linebuf[CV_VDP_WIDTH];
    memset(linebuf, 0x00, CV_VDP_WIDTH);

    /* Buffer sprite coincidence data (collision)
       This has to be handled separately from pixel data because the palette
       entry for an active pixel may be 0 (transparent). In this case it is
       still considered for collision calculation.
    */
    uint8_t cbuf[CV_VDP_WIDTH];
    memset(cbuf, 0x00, CV_VDP_WIDTH);

    for (int i = 0; i < 32; ++i) {
        /* Sprite Attribute Table Entry - Datasheet 2-25
        -------------------------------------
        |   7   6   5   4   3   2   1   0   | Bit Position
        |-----------------------------------|
        |             Y Position            | Byte 0: Vertical (Y) Position
        |-----------------------------------|
        |             X Position            | Byte 1: Horizontal (X) Position
        |-----------------------------------|
        |            Pattern Name           | Byte 2: Pattern Name (0-255)
        |-----------------------------------|
        |   EC  -   -   -   Colour Code     | Byte 3: Sprite Colour/Extra Clock
        -------------------------------------
        Notes: "Pattern Name" really refers to an index to pattern data.
               Extra Clock bit being set means decrement the X value by 32.
               Some Y positions have special meanings.
               Position 0,0 is the top left corner of the screen.
        */
        int y = vdp.vram[vdp.tbl_sattr + (i * 4)]; // "Partially signed"
        int x = vdp.vram[vdp.tbl_sattr + (i * 4) + 1];
        uint8_t pname = vdp.vram[vdp.tbl_sattr + (i * 4) + 2];
        uint8_t c = vdp.vram[vdp.tbl_sattr + (i * 4) + 3];

        // These bits are set every iteration regardless, but are only relevant
        // when the 5S bit is also set.
        vdp.stat &= ~0x1f; // Clear the FS bits (Fifth Sprite, 0-31)
        vdp.stat |= i & 0x1f; // Set FS bits to the current sprite index

        if (c & 0x80) // EC bit is set, reduce X by 32 pixels (Early Clock)
            x -= 32; // Allows sprites to be partially displayed on the left

        // If Y is 208, that sprite and all following sprites in the table are
        // not displayed. This is a Y value with a special meaning.
        if (y == 208)
            break;

        /* Wrap Y index if required - Datasheet says that a vertical
           displacement value of -31 to 0 allows a sprite to bleed in from the
           top edge of the backdrop. In this case it appears that 224 is equal
           to -31, in this "partially signed" context. 255 == 0, 254 == -1...
        */
        if (y > 224)
            y -= 256;

        /* Y index needs to be offset by 1. Datasheet says a value of -1 puts
           the sprite "butted up at the top of the screen, touching the backdrop
           area".
        */
        ++y;

        // If no rows of the sprite are actually on the scanline in question,
        // this iteration is finished.
        if ((y > vdp.line) || ((y + (sprsize << sprmag)) <= vdp.line))
            continue;

        if (++numspr == 5) { // There can only be 4 sprites per scanline
            vdp.stat |= 0x40; // Set the 5S bit (Fifth Sprite detected)
            break; // We're done here, so break the loop
        }

        // In the case of 16x16, to calculate the address in the Sprite
        // Generator table: ((pattern name) AND 252) x 8.
        if (sprsize == 16)
            pname &= 0xfc; // Do the masking here and the multiplication below

        // Calculate which row of the sprite pattern needs to be drawn
        int srow = vdp.line - y;

        // If it's magnified, divide the row in half so it will be drawn twice
        srow >>= sprmag;

        /* In the case of 8x8 sprites, there are 8 bytes for the sprite pattern,
           and there are 256 patterns in the sprite generator table.
           So simply multiply the sprite pattern by 8 to get the address.
        */
        uint8_t sppat = vdp.vram[vdp.tbl_spgen + (pname << 3) + srow];

        /* 16x16 Sprites - Datasheet 2-21
        ---------------------------------
        |  Quadrant A   |  Quadrant C   |   For 16x16 sprites, draw the pattern
        |     0x00      |     0x10      |   for Quadrants A+C or B+D on the same
        |      to       |      to       |   line. The address for the second set
        |     0x07      |     0x17      |   of pixels is offset by 16 (0x10).
        ---------------------------------
        |  Quadrant B   |  Quadrant D   |
        |     0x08      |     0x18      |
        |      to       |      to       |
        |     0x0f      |     0x1f      |
        ---------------------------------
        */

        // Loop through the sprite's pixel data - use shifts for magnification
        for (int p = 0; p < (sprsize << sprmag); ++p) {
            // Move to next iteration if the pixel is off screen, or empty
            if (((x + p) < -sprsize) || ((x + p) >= CV_VDP_WIDTH) || (c == 0))
                continue;

            // Handle the second pattern byte of 16x16 sprites
            if ((sprsize == 16) && (p == (8 << sprmag)))
                sppat = vdp.vram[(vdp.tbl_spgen + (pname << 3) + srow) | 0x10];

            // Check if a pixel needs to be drawn for this bit
            if (sppat & (0x80 >> ((p >> sprmag) & 7))) {
                // Set the C flag if a pixel has been drawn here already
                if (cbuf[x + p])
                    vdp.stat |= 0x20;
                else if (x + p >= 0) { // Otherwise draw a new pixel
                    linebuf[x + p] = c & 0x0f;

                    // Set collision data even if palette entry is transparent
                    cbuf[x + p] = 1;
                }
            }
        }
    }

    // Draw values to the line
    for (int i = 0; i < CV_VDP_WIDTH; ++i)
        if (linebuf[i]) // Draw non-transparent pixels
            jcv_vdp_pixel(palette[linebuf[i]], vdp.line, i + CV_VDP_OVERSCAN);
}

// Draw a scanline to the canvas
void jcv_vdp_exec(void) {
    if (jcv_vdp_rendering() && vdp.line < CV_VDP_HEIGHT) {
        jcv_vdp_bgline(); // Draw background
        if (!(vdp.ctrl[1] & 0x10)) // Do not draw sprites in Text Mode
            jcv_vdp_sprline(); // Draw sprites
    }
    else if (vdp.line < CV_VDP_HEIGHT) {
        jcv_vdp_bdline(vdp.line + CV_VDP_OVERSCAN);
    }

    // Increment the line number
    ++vdp.line;

    if (vdp.line == CV_VDP_HEIGHT) { // Enter VBLANK
        // Save the state of the Status Register INT bit
        uint8_t old_int = jcv_vdp_int();

        // Set the INT bit on the Status Register
        vdp.stat |= 0x80;

        /* Fire NMI if Register 1 GINT is set and Status Register INT was clear
           before entering VBLANK. This prevents the NMI from being fired if
           we're already in the interrupt service routine, and a read of the
           status register has not yet been done to clear the bit.
        */
        if (jcv_vdp_gint() && !old_int)
            jcv_z80_nmi();
    }

    // Start on the next frame when the end of this one is reached
    if (vdp.line == numscanlines) {
        vdp.line = 0;

        // Draw backdrop colour on the vertical overscan lines
        for (int i = 0; i < CV_VDP_OVERSCAN; ++i) {
            jcv_vdp_bdline(i);
            jcv_vdp_bdline(i + CV_VDP_HEIGHT + CV_VDP_OVERSCAN);
        }
    }
}

void jcv_vdp_state_load(uint8_t *st) {
    vdp.line = jcv_serial_pop16(st);
    vdp.dot = jcv_serial_pop16(st);
    jcv_serial_popblk(vdp.vram, st, SIZE_VRAM);
    vdp.addr = jcv_serial_pop16(st);
    vdp.dlatch = jcv_serial_pop8(st);
    vdp.wlatch = jcv_serial_pop8(st);
    for (size_t i = 0; i < 8; ++i) vdp.ctrl[i] = jcv_serial_pop8(st);
    vdp.stat = jcv_serial_pop8(st);
    vdp.tbl_col = jcv_serial_pop16(st);
    vdp.tbl_pgen = jcv_serial_pop16(st);
    vdp.tbl_pname = jcv_serial_pop16(st);
    vdp.tbl_sattr = jcv_serial_pop16(st);
    vdp.tbl_spgen = jcv_serial_pop16(st);
}

void jcv_vdp_state_save(uint8_t *st) {
    jcv_serial_push16(st, vdp.line);
    jcv_serial_push16(st, vdp.dot);
    jcv_serial_pushblk(st, vdp.vram, SIZE_VRAM);
    jcv_serial_push16(st, vdp.addr);
    jcv_serial_push8(st, vdp.dlatch);
    jcv_serial_push8(st, vdp.wlatch);
    for (size_t i = 0; i < 8; ++i) jcv_serial_push8(st, vdp.ctrl[i]);
    jcv_serial_push8(st, vdp.stat);
    jcv_serial_push16(st, vdp.tbl_col);
    jcv_serial_push16(st, vdp.tbl_pgen);
    jcv_serial_push16(st, vdp.tbl_pname);
    jcv_serial_push16(st, vdp.tbl_sattr);
    jcv_serial_push16(st, vdp.tbl_spgen);
}
