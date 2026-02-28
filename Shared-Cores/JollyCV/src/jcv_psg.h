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

#ifndef JCV_PSG_H
#define JCV_PSG_H

typedef struct _cv_psg_t {
    uint8_t clatch; // Channel latch tells which channel's registers to write
    uint8_t attenuator[4]; // Four attenuators control volume on four channels
    uint16_t frequency[3]; // Three frequency registers for Tone Generators
    uint8_t noise; // One register for the Noise Generator
    uint16_t lfsr; // Linear Feedback Shift Register (15 bits on ColecoVision)
    uint16_t counter[4]; // Period Counter
    int16_t output[4]; // Per-channel output volumes for mixing
    uint8_t freqff; // Four bits for four channels, 0 = Positive, 1 = Negative
} cv_psg_t;

void jcv_psg_set_buffer(int16_t*);
void jcv_psg_reset_buffer(void);

void jcv_psg_init(void);
void jcv_psg_wr(uint8_t);
size_t jcv_psg_exec(void);

void jcv_psg_state_load(uint8_t*);
void jcv_psg_state_save(uint8_t*);

#endif
