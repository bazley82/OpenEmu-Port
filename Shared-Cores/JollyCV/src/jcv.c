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

#include <stdint.h>
#include <stddef.h>

#include "jcv.h"
#include "jcv_memio.h"
#include "jcv_mixer.h"
#include "jcv_psg.h"
#include "jcv_sgmpsg.h"
#include "jcv_vdp.h"
#include "jcv_z80.h"

/* NTSC Timing
Z80 cycles per audio sample at 48000Hz (16 CPU cycles per PSG cycle):
    (3579545 / x) / 16 = 48000
    x = 4.6608659

Z80 cycles per frame (2 CPU cycles per 3 VDP cycles):
    89603.5 * 2/3 = 59735.66667

Z80 cycles per scanline:
    59735.66667 / 262 = 227.99873 (~228)

VDP cycles per frame:
    342 cycles per line, 262 lines, skip final cycle of frame every other frame
    89604 or 89603 (89603.5)

PSG cycles per frame:
    59735.66667 / 16 = 3733.4792 (~224KHz)
*/

#define DIV_PSG 16 // PSG Clock Divider
#define Z80_CYC_LINE 228 // Z80 CPU cycles per scanline (227.99873)

static size_t numscanlines = CV_VDP_SCANLINES;
static size_t psgcycs = 0;
static size_t psgsamps = 0;
static size_t sgmpsgsamps = 0;

// Set the region
void jcv_set_region(uint8_t region) {
    // 313 scanlines for PAL, 262 scanlines for NTSC (192 visible for both)
    numscanlines = region ? CV_VDP_SCANLINES_PAL : CV_VDP_SCANLINES;
    jcv_mixer_set_region(region);
    jcv_vdp_set_region(region);
}

// Initialize
void jcv_init(void) {
    jcv_memio_init();
    jcv_psg_init();
    jcv_sgmpsg_init();
    jcv_mixer_init();
    jcv_vdp_init();
    jcv_z80_init();
}

// Deinitialize
void jcv_deinit(void) {
    jcv_memio_deinit();
    jcv_mixer_deinit();
}

// Reset the system
void jcv_reset(int hard) {
    if (hard) { } // Currently unused
    jcv_memio_init(); // Init does the same thing reset needs to do
    jcv_psg_init();
    jcv_sgmpsg_init();
    jcv_vdp_init();
    jcv_z80_reset();
}

// Run emulation for one frame
void jcv_exec(void) {
    // Keep track of the number of samples generated this frame
    psgsamps = 0;
    sgmpsgsamps = 0;

    // Restore the leftover cycle count
    uint32_t extcycs = jcv_z80_cyc_restore();

    // Run scanline-based iterations of emulation until a frame is complete
    for (size_t i = 0; i < numscanlines; ++i) {
        // Set the number of cycles required to complete this scanline
        size_t reqcycs = Z80_CYC_LINE - extcycs;

        // Count cycles for an iteration (usually one instruction)
        size_t itercycs = 0;

        // Count the total cycles run in a scanline
        size_t linecycs = 0;

        // Run CPU instructions until enough have been run for one scanline
        while (linecycs < reqcycs) {
            itercycs = jcv_z80_exec(); // Run a single CPU instruction
            linecycs += itercycs; // Add the number of cycles to the total

            for (size_t s = 0; s < itercycs; ++s) { // Catch PSGs up to the CPU
                if (++psgcycs % DIV_PSG == 0) {
                    psgsamps += jcv_psg_exec();
                    sgmpsgsamps += jcv_sgmpsg_exec();
                    psgcycs = 0;
                }
            }
        }

        extcycs = linecycs - reqcycs; // Store extra cycle count

        jcv_vdp_exec(); // Draw a scanline of pixel data
    }

    // Resample audio and push to the frontend
    jcv_mixer_resamp(psgsamps, sgmpsgsamps);

    // Store the leftover cycle count
    jcv_z80_cyc_store(extcycs);
}
