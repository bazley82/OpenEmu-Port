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

#ifndef JCV_VDP_H
#define JCV_VDP_H

#define CV_VDP_OVERSCAN 8
#define CV_VDP_WIDTH 256
#define CV_VDP_HEIGHT 192
#define CV_VDP_WIDTH_OVERSCAN 272
#define CV_VDP_HEIGHT_OVERSCAN 208
#define CV_VDP_SCANLINES 262
#define CV_VDP_SCANLINES_PAL 313

#define SIZE_VRAM 0x4000

typedef struct _cv_vdp_t {
    uint16_t line; // Line currently being drawn
    uint16_t dot; // Dot currently being drawn
    uint8_t vram[SIZE_VRAM]; // 16K VRAM
    uint16_t addr; // Memory Address - 14 bit address
    uint8_t dlatch; // Data Latch (general purpose 8-bit data register)
    uint8_t wlatch; // Write Latch
    uint8_t ctrl[8]; // 8 Control Registers - write only
    uint8_t stat; // Status Register - read only
    uint16_t tbl_col; // Address for Colour table
    uint16_t tbl_pgen; // Address for Pattern Generator table
    uint16_t tbl_pname; // Address for Pattern Name table
    uint16_t tbl_sattr; // Address for Sprite Attribute table
    uint16_t tbl_spgen; // Addresss for Sprite Generator table
} cv_vdp_t;

void jcv_vdp_init(void);

void jcv_vdp_set_buffer(uint32_t*);
void jcv_vdp_set_palette(uint8_t);
void jcv_vdp_set_region(uint8_t);

uint8_t jcv_vdp_rd_data(void);
uint8_t jcv_vdp_rd_stat(void);

void jcv_vdp_wr_ctrl(uint8_t);
void jcv_vdp_wr_data(uint8_t);

void jcv_vdp_exec(void);

void jcv_vdp_state_load(uint8_t*);
void jcv_vdp_state_save(uint8_t*);

#endif
