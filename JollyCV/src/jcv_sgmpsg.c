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

// ColecoVision Super Game Module PSG - General Instrument AY-3-8910

#include <stddef.h>
#include <stdint.h>

#include "jcv_serial.h"
#include "jcv_sgmpsg.h"

static const int16_t vtable[16] = { // Volume Table
    0,       40,      60,     86,    124,    186,    264,    440,
    518,    840,    1196,   1526,   2016,   2602,   3300,   4096,
};

static cv_sgmpsg_t psg; // SGM PSG Context
static int16_t *psgbuf = NULL; // Buffer for raw PSG output samples
static size_t bufpos = 0; // Keep track of the position in the PSG output buffer

// Reset the Envelope step and volume depending on the currently selected shape
static inline void jcv_sgmpsg_env_reset(void) {
    psg.estep = 0; // Reset the step counter

    if (psg.eseg) { // Segment 1
        switch (psg.reg[13]) {
            case 8: case 11: case 13: case 14: { // Start from the top
                psg.evol = 15;
                break;
            }
            default: { // Start from the bottom
                psg.evol = 0;
                break;
            }
        }
    }
    else { // Segment 0
        // If the Attack bit is set, start from the bottom. Otherwise, the top.
        psg.evol = psg.reg[13] & 0x04 ? 0 : 15;
    }
}

// Set the pointer to the sample buffer
void jcv_sgmpsg_set_buffer(int16_t *ptr) {
    psgbuf = ptr;
}

// Reset position of the buffer
void jcv_sgmpsg_reset_buffer(void) {
    bufpos = 0;
}

// Set initial values
void jcv_sgmpsg_init(void) {
    // Registers
    for (int i = 0; i < 16; ++i)
        psg.reg[i] = 0x00;

    // Latched Register
    psg.rlatch = 0x00;

    // Tone Periods, Tone Counters, Amplitude, Sign bits
    for (int i = 0; i < 3; ++i) {
        psg.tperiod[i] = 0x0000;
        psg.tcounter[i] = 0x0000;
        psg.amplitude[i] = 0x00;
        psg.sign[i] = 0x00;
    }

    // Noise Period, Noise Counter
    psg.nperiod = 0x00;
    psg.ncounter = 0x0000;

    // Seed the Noise RNG Shift Register
    psg.nshift = 1;

    // Envelope Period, Counter, Segment, Step, and Volume
    psg.eperiod = 0x0000;
    psg.ecounter = 0x0000;
    psg.eseg = 0x00;
    psg.estep = 0x00;
    psg.evol = 0x00;

    // Enable bits for Tone, Noise, and Envelope
    for (int i = 0; i < 3; ++i) {
        psg.tdisable[i] = 0x00;
        psg.ndisable[i] = 0x00;
        psg.emode[i] = 0x00;
    }
}

// Read from the currently latched Control Register
uint8_t jcv_sgmpsg_rd(void) {
    return psg.reg[psg.rlatch];
}

// Write to the currently latched Control Register
void jcv_sgmpsg_wr(uint8_t data) {
    /* Registers
    |---|-----------------------------------------------|
    | R |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
    |---|-----------------------------------------------|
    | 0 |                8-bit fine tune                | Channel A Tone Period
    | 1 |  -     -     -     -  |   4-bit coarse tune   |
    |---|-----------------------------------------------|
    | 2 |                8-bit fine tune                | Channel B Tone Period
    | 3 |  -     -     -     -  |   4-bit coarse tune   |
    |---|-----------------------------------------------|
    | 4 |                8-bit fine tune                | Channel C Tone Period
    | 5 |  -     -     -     -  |   4-bit coarse tune   |
    |---|-----------------------------------------------|
    | 6 |  -     -     -  |    5-bit period control     | Noise Period
    |---|-----------------------------------------------|
    | 7 | IOB | IOA | NC  | NB  | NA  | TC  | TB  | TA  | Enable IO/Noise/Tone
    |---|-----------------------------------------------|
    | 8 |  -     -     -  |  M  | L3  | L2  | L1  | L0  | Channel A Amplitude
    |---|-----------------------------------------------|
    | 9 |  -     -     -  |  M  | L3  | L2  | L1  | L0  | Channel B Amplitude
    |---|-----------------------------------------------|
    |10 |  -     -     -  |  M  | L3  | L2  | L1  | L0  | Channel C Amplitude
    |---|-----------------------------------------------|
    |11 |                8-bit fine tune                | Envelope Period
    |12 |               8-bit coarse tune               |
    |---|-----------------------------------------------|
    |13 |  -     -     -     -  |CONT | ATT | ALT |HOLD | Envelope Shape/Cycle
    |---|-----------------------------------------------|
    |14 |          8-bit Parallel IO on Port A          | IO Port A Data Store
    |---|-----------------------------------------------|
    |15 |          8-bit Parallel IO on Port B          | IO Port B Data Store
    |---|-----------------------------------------------|
    */

    // Masks to avoid writing "Don't Care" bits
    uint8_t dcmask[16] = {
        0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0xff,
        0x1f, 0x1f, 0x1f, 0xff, 0xff, 0x0f, 0xff, 0xff,
    };

    // Write data to the latched register
    psg.reg[psg.rlatch] = data & dcmask[psg.rlatch];

    switch (psg.rlatch) {
        /* Tone Periods are 12-bit values comprising 8 bits from the first
           register, 4 bits from the second register. Value is for half period.
           The lowest period for tones is 1, so if 0 is set, change it to 1.
        */
        case 0: case 1: { // Channel A Tone Period
            psg.tperiod[0] = psg.reg[0] | (psg.reg[1] << 8);
            if (psg.tperiod[0] == 0)
                psg.tperiod[0] = 1;
            break;
        }
        case 2: case 3: { // Channel B Tone Period
            psg.tperiod[1] = psg.reg[2] | (psg.reg[3] << 8);
            if (psg.tperiod[1] == 0)
                psg.tperiod[1] = 1;
            break;
        }
        case 4: case 5: { // Channel C Tone Period
            psg.tperiod[2] = psg.reg[4] | (psg.reg[5] << 8);
            if (psg.tperiod[2] == 0)
                psg.tperiod[2] = 1;
            break;
        }
        case 6: { // Noise Period
            psg.nperiod = psg.reg[6];
            if (psg.nperiod == 0) // As with Tones, lowest period value is 1.
                psg.nperiod = 1;
            break;
        }
        case 7: { // Enable IO/Noise/Tone
            // Register 7's Enable bits are actually Disable bits.
            psg.tdisable[0] = (psg.reg[7] >> 0) & 0x01;
            psg.tdisable[1] = (psg.reg[7] >> 1) & 0x01;
            psg.tdisable[2] = (psg.reg[7] >> 2) & 0x01;
            psg.ndisable[0] = (psg.reg[7] >> 3) & 0x01;
            psg.ndisable[1] = (psg.reg[7] >> 4) & 0x01;
            psg.ndisable[2] = (psg.reg[7] >> 5) & 0x01;
            break;
        }
        case 8: { // Channel A Amplitude
            psg.amplitude[0] = data & 0x0f;
            psg.emode[0] = (data >> 4) & 0x01;
            break;
        }
        case 9: { // Channel B Amplitude
            psg.amplitude[1] = data & 0x0f;
            psg.emode[1] = (data >> 4) & 0x01;
            break;
        }
        case 10: { // Channel C Amplitude
            psg.amplitude[2] = data & 0x0f;
            psg.emode[2] = (data >> 4) & 0x01;
            break;
        }
        case 11: case 12: { // Envelope Period
            psg.eperiod = psg.reg[11] | (psg.reg[12] << 8);
            break;
        }
        case 13: { // Envelope Shape
            // Reset all Envelope related variables when Register 13 is written
            psg.ecounter = 0;
            psg.eseg = 0;
            jcv_sgmpsg_env_reset();
            break;
        }
        /* Nothing really needs to be done for the IO Port Data Store Registers,
           so skip cases 14 and 15.
        */
        default: {
            break;
        }
    }
}

// Set the latched Control Register
void jcv_sgmpsg_set_reg(uint8_t r) {
    psg.rlatch = r;
}

// Execute a PSG cycle
size_t jcv_sgmpsg_exec(void) {
    // Clock Tone Counters for Channels A, B, and C
    for (int i = 0; i < 3; ++i) {
        if (++psg.tcounter[i] >= psg.tperiod[i]) {
            psg.tcounter[i] = 0;
            psg.sign[i] ^= 1;
        }
    }

    // Clock Noise Counter
    if (++psg.ncounter >= (psg.nperiod << 1)) {
        psg.ncounter = 0;
        /* The Noise Random Number Generator is a 17-bit shift register, whose
           input is bit 0 XOR bit 3. The result of this operation is output at
           bit 16 as bit 1 becomes the new bit 0, which will determine whether
           to output noise when a sample is generated.
        */
        psg.nshift = (psg.nshift >> 1) |
            (((psg.nshift ^ (psg.nshift >> 3)) & 0x01) << 16);
    }

    // Clock Envelope Counter
    if (++psg.ecounter >= (psg.eperiod << 1)) {
        psg.ecounter = 0;

        /* Envelope Shape
           The bits from 3 to 0 represent Continue, Attack, Alternate, and Hold.
           For Continue values of 0, the bottom two bits are irrelevant, meaning
           there are only 2 possible shapes for the first 8 numerical values.
           00xx: \____
           01xx: /|____
           1000: \|\|\|     1001: \_____     1010: \/\/\/     1011: \|----
           1100: /|/|/|     1101: /-----     1110: /\/\/\     1111: /|____
        */
        if (psg.estep) { // Do not change the envelope's volume for the 0th step
            if (psg.eseg) { // Second half of the envelope shape
                if ((psg.reg[13] == 10) || (psg.reg[13] == 12))
                    ++psg.evol; // Count Up
                else if ((psg.reg[13] == 8) || (psg.reg[13] == 14))
                    --psg.evol; // Count Down
                // Otherwise, simply hold the current value (no else statement)
            }
            else { // First half of the envelope shape
                if (psg.reg[13] & 0x04) // Attack is set - Count Up
                    ++psg.evol;
                else // Count Down
                    --psg.evol;
            }
        }

        // Reset and start the new Segment if this is the last Envelope Step
        if (++psg.estep >= 16) {
            if ((psg.reg[13] & 0x09) == 0x08) // Switch Envelope Segment
                psg.eseg ^= 1;
            else // Hold the current Segment for 0-7, 9, 11, 13, 15
                psg.eseg = 1;
            jcv_sgmpsg_env_reset();
        }
    }

    int16_t vol = 0; // Initial output volume of this sample

    for (int i = 0; i < 3; ++i) {
        /* Determine whether to output a volume for this channel. The logic here
           is unintuitive. From the datasheet:
             "Disabling noise and tone does _not_ turn off a channel. Turning a
             channel off can only be accomplished by writing all zeroes into the
             corresponding Amplitude Control register."
           If both tone and noise disable bits are set, the output value will
           effectively be silence because the waveform will not oscillate. If
           either only the tone or only the noise disable bit is set, it will
           determine whether tone or noise is output. If neither are set, sound
           will only be output when both the noise shift register bit 0 is set
           and the tone is in the second half of the period.
        */
        uint8_t out = (psg.tdisable[i] | psg.sign[i]) &
            (psg.ndisable[i] | (psg.nshift & 0x01));

        /* If the envelope mode bit is set for this channel, output variable
           level amplitude (envelope step), otherwise output the fixed level
           amplitude value.
        */
        if (out)
            vol += psg.emode[i] ? vtable[psg.evol] : vtable[psg.amplitude[i]];
    }

    // Add the mixed sample to the output buffer and increment the position
    psgbuf[bufpos++] = vol;

    return 1; // Return 1, signifying that a sample has been generated
}

void jcv_sgmpsg_state_load(uint8_t *st) {
    for (size_t i = 0; i < 16; ++i) psg.reg[i] = jcv_serial_pop8(st);
    psg.rlatch = jcv_serial_pop8(st);
    for (size_t i = 0; i < 3; ++i) psg.tperiod[i] = jcv_serial_pop16(st);
    for (size_t i = 0; i < 3; ++i) psg.tcounter[i] = jcv_serial_pop16(st);
    for (size_t i = 0; i < 3; ++i) psg.amplitude[i] = jcv_serial_pop8(st);
    psg.nperiod = jcv_serial_pop8(st);
    psg.ncounter = jcv_serial_pop16(st);
    psg.nshift = jcv_serial_pop32(st);
    psg.eperiod = jcv_serial_pop16(st);
    psg.ecounter = jcv_serial_pop16(st);
    psg.eseg = jcv_serial_pop8(st);
    psg.estep = jcv_serial_pop8(st);
    psg.evol = jcv_serial_pop8(st);
    for (size_t i = 0; i < 3; ++i) psg.tdisable[i] = jcv_serial_pop8(st);
    for (size_t i = 0; i < 3; ++i) psg.ndisable[i] = jcv_serial_pop8(st);
    for (size_t i = 0; i < 3; ++i) psg.emode[i] = jcv_serial_pop8(st);
    for (size_t i = 0; i < 3; ++i) psg.sign[i] = jcv_serial_pop8(st);
}

void jcv_sgmpsg_state_save(uint8_t *st) {
    for (size_t i = 0; i < 16; ++i) jcv_serial_push8(st, psg.reg[i]);
    jcv_serial_push8(st, psg.rlatch);
    for (size_t i = 0; i < 3; ++i) jcv_serial_push16(st, psg.tperiod[i]);
    for (size_t i = 0; i < 3; ++i) jcv_serial_push16(st, psg.tcounter[i]);
    for (size_t i = 0; i < 3; ++i) jcv_serial_push8(st, psg.amplitude[i]);
    jcv_serial_push8(st, psg.nperiod);
    jcv_serial_push16(st, psg.ncounter);
    jcv_serial_push32(st, psg.nshift);
    jcv_serial_push16(st, psg.eperiod);
    jcv_serial_push16(st, psg.ecounter);
    jcv_serial_push8(st, psg.eseg);
    jcv_serial_push8(st, psg.estep);
    jcv_serial_push8(st, psg.evol);
    for (size_t i = 0; i < 3; ++i) jcv_serial_push8(st, psg.tdisable[i]);
    for (size_t i = 0; i < 3; ++i) jcv_serial_push8(st, psg.ndisable[i]);
    for (size_t i = 0; i < 3; ++i) jcv_serial_push8(st, psg.emode[i]);
    for (size_t i = 0; i < 3; ++i) jcv_serial_push8(st, psg.sign[i]);
}
