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

#ifndef JCV_SGMPSG_H
#define JCV_SGMPSG_H

typedef struct _cv_sgmpsg_t {
    uint8_t reg[16]; // 16 Read/Write 8-bit registers
    uint8_t rlatch; // Register that is currently selected

    uint16_t tperiod[3]; // Periods for Tones A, B, and C
    uint16_t tcounter[3]; // Counters for Tones A, B, and C
    uint8_t amplitude[3]; // Amplitudes for Tones A, B, and C

    uint8_t nperiod; // Noise Period
    uint16_t ncounter; // Noise Counter
    uint32_t nshift; // Noise Random Number Generator Shift Register (17-bit)

    uint16_t eperiod; // Envelope Period
    uint16_t ecounter; // Envelope Counter
    uint8_t eseg; // Envelope Segment: Which half of the cycle
    uint8_t estep; // Envelope Step
    uint8_t evol; // Envelope Volume

    uint8_t tdisable[3]; // Disable bit for Tones A, B, and C
    uint8_t ndisable[3]; // Disable bit for Noise on Channels A, B, and C
    uint8_t emode[3]; // Envelope Mode Enable bit for Tones A, B, and C

    uint8_t sign[3]; // Signify whether the waveform is high or low
} cv_sgmpsg_t;

void jcv_sgmpsg_set_buffer(int16_t*);
void jcv_sgmpsg_reset_buffer(void);
void jcv_sgmpsg_init(void);
uint8_t jcv_sgmpsg_rd(void);
void jcv_sgmpsg_wr(uint8_t data);
void jcv_sgmpsg_set_reg(uint8_t);
size_t jcv_sgmpsg_exec(void);

void jcv_sgmpsg_state_load(uint8_t*);
void jcv_sgmpsg_state_save(uint8_t*);

#endif
