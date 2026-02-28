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

// ColecoVision PSG - Texas Instruments SN76489AN

#include <stddef.h>
#include <stdint.h>

#include "jcv_psg.h"
#include "jcv_serial.h"

#define LFSRSHIFT 14 // Linear Feedback Shift Register is 15 bits, so shift 14
#define NOISETAP 0x0003 // Tapped bits for ColecoVision are 0 and 1

// Based on smspower documentation, divided by 4 and tweaked
static const int16_t vtable[16] = { // Volume Table
    0x1fff, 0x196b, 0x1431, 0x100a, 0x0cbd, 0x0a1f, 0x080a, 0x066a,
    0x0512, 0x0407, 0x0333, 0x028b, 0x0205, 0x019b, 0x0146, 0x0000,
};

static cv_psg_t psg; // PSG Context
static int16_t *psgbuf = NULL; // Buffer for raw PSG output samples
static size_t bufpos = 0; // Keep track of the position in the PSG output buffer

// Set the pointer to the sample buffer
void jcv_psg_set_buffer(int16_t *ptr) {
    psgbuf = ptr;
}

// Grab the pointer to the PSG's buffer
void jcv_psg_reset_buffer(void) {
    bufpos = 0;
}

// Set initial values
void jcv_psg_init(void) {
    psg.clatch = 0x00; // Channel Latch starts at Tone Channel 0

    for (int i = 0; i < 4; ++i) {
        psg.attenuator[i] = 0x0f; // Silence
        psg.counter[i] = 0x00; // Count starting from 0
    }

    // Set the frequency and noise registers to 0
    psg.frequency[0] = psg.frequency[1] = psg.frequency[2] = psg.noise = 0x00;

    psg.lfsr = 1 << LFSRSHIFT; // Seed the noise shift register
    psg.freqff = 0x00; // Frequency flip-flop bits start at 0 (Positive)
}

// Write to PSG Control Registers
void jcv_psg_wr(uint8_t data) {
    /* Register Writes
    There are two types of register writes, referred to in the smspower
    documentation as LATCH/DATA and DATA.

    Bit 7 being set in the input byte signifies a LATCH/DATA byte:
    |-------------------------------|  LATCH/DATA bytes set the channel latch
    | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |  so that subsequent writes of DATA bytes
    |-------------------------------|  will affect the correct channel. Since
    | L | Register  |    Data       |  there are 8 registers, 3 bits are used.
    |-------------------------------|  The 4 least significant bits are data.

    Bit 7 being unset signifies a DATA byte:
    |-------------------------------|  For writes to Frequency Registers, the
    | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |  6 least significant bits become the
    |-------------------------------|  upper 6 bits of the 10 bit frequency
    | l | - |         Data          |  period. For other registers, the data is
    |-------------------------------|  handled the same as LATCH/DATA bytes.

    Register Addresses:
    |--------------------------------|
    | 2 | 1 | 0 |  Control Register  |
    |--------------------------------|
    | 0 | 0 | 0 | Tone 0 Frequency   | Frequency value determines half of the
    | 0 | 0 | 1 | Tone 0 Attenuation | period. Volume is controlled by
    | 0 | 1 | 0 | Tone 1 Frequency   | attenuation values (0 to 16). In this
    | 0 | 1 | 1 | Tone 1 Attenuation | emulator, these are values in a table,
    | 1 | 0 | 0 | Tone 2 Frequency   | but in reality, a formula determines the
    | 1 | 0 | 1 | Tone 2 Attenuation | value (see smspower documentation). Full
    | 1 | 1 | 0 | Noise Control      | attenuation means silence, no attenuation
    | 1 | 1 | 1 | Noise Attenuation  | means full volume.
    |--------------------------------|

    Noise Register:
    |-------------------------------|  Only 3 bits are used. "F" is feedback,
    | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |  and controls periodic noise (0) or white
    |-------------------------------|  noise (1). "Rate" is the shift rate:
    | -   -   -   -   - | F | Rate  |  0x00 = N/512, 0x01 = N/1024,
    |-------------------------------|  0x02 = N/2048, 0x03 = Tone 2 Freq Counter
    */
    if (data & 0x80) // LATCH/DATA byte - update the latch
        psg.clatch = data; // Record the data in the channel latch

    // For convenience, store the channel as a variable
    uint8_t chan = (psg.clatch & 0x60) >> 5; // Channel (2 bits, 0-3)

    if (psg.clatch & 0x10) { // Attenuator Registers for channels 0-3
        // (DDDDDD)dddd = (--vvvv)vvvv
        psg.attenuator[chan] = data & 0x0f;
    }
    else { // Frequency/Noise Registers
        if (chan < 3) { // Frequency Registers for channels 0-2
            // DDDDDDdddd = cccccccccc
            psg.frequency[chan] = data & 0x80 ? // Detect byte type
                (psg.frequency[chan] & 0x03f0) | (data & 0x0f) : // LATCH/DATA
                ((psg.frequency[chan] & 0x0f) | (data << 4)) & 0x03ff; // DATA
        }
        else if (chan == 3) { // Noise Register for channel 3
            // (DDDDDD)dddd = (---trr)-trr
            psg.noise = data & 0x07;
            // Whenever the noise control register is changed, the shift
            // register is cleared/reseeded.
            psg.lfsr = 1 << LFSRSHIFT;
        }
    }
}

static inline uint16_t parity(uint16_t v) {
    /* This technique is used, adapted for 16-bit values:
       https://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
    */
    v ^= v >> 8;
    v ^= v >> 4;
    v &= 0xf;
    return (0x6996 >> v) & 1;
}

// Execute a PSG cycle
size_t jcv_psg_exec(void) {
    // Tone Generators
    for (size_t i = 0; i < 3; ++i) {
        // Each clock cycle, the counter is decremented (if it is non-zero)
        if (psg.counter[i] > 0)
            --psg.counter[i]; // Decrement the period counter

        if (psg.counter[i] == 0) {
            /* When the tone counter decrements to zero, it is reloaded with
               the value of the corresponding frequency register. In order to
               produce a wave, it must oscillate. The value in the frequency
               register actually represents half of the period (the period is
               double the value in the register). PCM sample playback uses a
               special feature of the SN76489's tone generators: when the value
               is set to 1, they output a DC offset value corresponding to the
               volume level. PCM is done by rapidly changing the volume level.
            */
            psg.counter[i] = psg.frequency[i];

            // Update the volume of the output channel
            psg.output[i] = vtable[psg.attenuator[i]];

            // Flip the frequency flip-flop for the channel (sign/polarity bit)
            psg.freqff ^= 1 << i;

            // Set the waveform high or low
            if (psg.freqff & (1 << i))
                psg.output[i] = 0;
        }
    }

    // Noise Generator
    if (psg.counter[3] > 0) // If it is already zero, no need to decrement
        --psg.counter[3];

    // Update the volume value for the noise output channel
    psg.output[3] = (psg.lfsr & 0x01) * vtable[psg.attenuator[3]];

    if (psg.counter[3] == 0) {
        /* Set the shift rate or use the Tone Generator 2 frequency
           If the value of the lowest two bits of the noise register is 3, then
           use the value of Tone Generator 2's frequency. Otherwise shift 0x10
           left by the value of the register.
        */
        psg.counter[3] = (psg.noise & 0x03) == 0x03 ?
            psg.frequency[2] : 0x10 << (psg.noise & 0x03);

        psg.freqff ^= 0x08; // Flip the bit for this channel

        /* White Noise:
        ->|1|0|0|0|0|0|0|0|0|0|0|0|0|0|0|  Bits 0 and 1 are the Tapped Bits.
        |                 __         | |   Linear Feedback Shift Register is
        |__________(XOR)_/  //-------- |   XORed against the Tapped Bits to
                     0   \__\\----------   determine what value is inserted at
        Becomes:                           bit 14 after shifting the LFSR right.
          |0|1|0|0|0|0|0|0|0|0|0|0|0|0|0| --> |0| (Discarded)

        Parity plays a role in the value, which will be 1 if an odd number of
        bits are set, and 0 if an even number are set.

        Periodic Noise:
        ->|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|  |1|
        |                                  /
        |-<-------<-------<-------<-------|

        Becomes:
          |1|0|0|0|0|0|0|0|0|0|0|0|0|0|0| --> |0| (Discarded)

        Bit 14 set, Bit 0 discarded
        */
        if (psg.freqff & 0x08) { // Adjust if frequency flip-flop bit is set
            // First shift the register, then insert the proper bit at Bit 14
            psg.lfsr = (psg.lfsr >> 1) | ((psg.noise & 0x04) ?
                (parity(psg.lfsr & NOISETAP) << LFSRSHIFT) : // White Noise
                ((psg.lfsr & 0x01) << LFSRSHIFT)); // Periodic Noise
        }
    }

    // Mix the channel output volumes into a single sample
    psgbuf[bufpos++] =
        psg.output[0] + psg.output[1] + psg.output[2] + psg.output[3];

    return 1; // Return 1, signifying that a sample has been generated
}

void jcv_psg_state_load(uint8_t *st) {
    psg.clatch = jcv_serial_pop8(st);
    for (size_t i = 0; i < 4; ++i) psg.attenuator[i] = jcv_serial_pop8(st);
    for (size_t i = 0; i < 3; ++i) psg.frequency[i] = jcv_serial_pop16(st);
    psg.noise = jcv_serial_pop8(st);
    psg.lfsr = jcv_serial_pop16(st);
    for (size_t i = 0; i < 4; ++i) psg.counter[i] = jcv_serial_pop16(st);
    for (size_t i = 0; i < 4; ++i) psg.output[i] = jcv_serial_pop16(st);
    psg.freqff = jcv_serial_pop8(st);
}

void jcv_psg_state_save(uint8_t *st) {
    jcv_serial_push8(st, psg.clatch);
    for (size_t i = 0; i < 4; ++i) jcv_serial_push8(st, psg.attenuator[i]);
    for (size_t i = 0; i < 3; ++i) jcv_serial_push16(st, psg.frequency[i]);
    jcv_serial_push8(st, psg.noise);
    jcv_serial_push16(st, psg.lfsr);
    for (size_t i = 0; i < 4; ++i) jcv_serial_push16(st, psg.counter[i]);
    for (size_t i = 0; i < 4; ++i) jcv_serial_push16(st, psg.output[i]);
    jcv_serial_push8(st, psg.freqff);
}
