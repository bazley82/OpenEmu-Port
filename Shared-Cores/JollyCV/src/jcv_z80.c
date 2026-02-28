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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "jcv_memio.h"
#include "jcv_serial.h"
#include "jcv_z80.h"

#include "z80.h"

static z80 z80ctx;
static uint32_t extracycs = 0;
static uint32_t delaycycs = 0;

// Memory Read
static uint8_t read_byte(void *userdata, uint16_t addr) {
    if (userdata) { } // Unused
    return jcv_mem_rd(addr);
}

// Memory Write
static void write_byte(void *userdata, uint16_t addr, uint8_t data) {
    if (userdata) { } // Unused
    jcv_mem_wr(addr, data);
}

// IO Port Read
static uint8_t port_in(z80 *z, uint16_t port) {
    if (z) { } // Unused
    return jcv_io_rd(port & 0xff);
}

// IO Port Write
static void port_out(z80 *z, uint16_t port, uint8_t data) {
    if (z) { } // Unused
    jcv_io_wr(port & 0xff, data);
}

// Store extra cycle count
void jcv_z80_cyc_store(uint32_t cycs) {
    extracycs = cycs;
}

// Retrieve stored extra cycle count
uint32_t jcv_z80_cyc_restore(void) {
    uint32_t ret = extracycs;
    extracycs = 0;
    return ret;
}

// Initialize the Z80
void jcv_z80_init(void) {
    z80_init(&z80ctx);
    z80ctx.read_byte = &read_byte;
    z80ctx.write_byte = &write_byte;
    z80ctx.port_in = &port_in;
    z80ctx.port_out = &port_out;
}

// Reset the Z80
void jcv_z80_reset(void) {
    jcv_z80_init();
}

// Generate an Interrupt
void jcv_z80_irq(uint8_t data) {
    z80_pulse_irq(&z80ctx, data);
}

// Generate a Non-Maskable Interrupt
void jcv_z80_nmi(void) {
    z80_pulse_nmi(&z80ctx);
}

// Delay the Z80's execution by a requested number of cycles
void jcv_z80_delay(uint32_t delay) {
    delaycycs += delay;
}

// Run a single Z80 instruction
uint32_t jcv_z80_exec(void) {
    uint32_t retcyc = z80_step(&z80ctx);

    if (delaycycs) {
        retcyc += delaycycs;
        delaycycs = 0;
    }

    return retcyc;
}

// Run Z80 instructions until at least the requested number of cycles have run
uint32_t jcv_z80_run(uint32_t cycles) {
    uint32_t retcyc = z80_step_n(&z80ctx, cycles);

    if (delaycycs) {
        retcyc += delaycycs;
        delaycycs = 0;
    }

    return retcyc;
}

// Restore the Z80's state from external data
void jcv_z80_state_load(uint8_t *st) {
    z80ctx.pc = jcv_serial_pop16(st);
    z80ctx.sp = jcv_serial_pop16(st);
    z80ctx.ix = jcv_serial_pop16(st);
    z80ctx.iy = jcv_serial_pop16(st);
    z80ctx.mem_ptr = jcv_serial_pop16(st);
    z80ctx.a = jcv_serial_pop8(st);
    z80ctx.f = jcv_serial_pop8(st);
    z80ctx.b = jcv_serial_pop8(st);
    z80ctx.c = jcv_serial_pop8(st);
    z80ctx.d = jcv_serial_pop8(st);
    z80ctx.e = jcv_serial_pop8(st);
    z80ctx.h = jcv_serial_pop8(st);
    z80ctx.l = jcv_serial_pop8(st);
    z80ctx.a_ = jcv_serial_pop8(st);
    z80ctx.f_ = jcv_serial_pop8(st);
    z80ctx.b_ = jcv_serial_pop8(st);
    z80ctx.c_ = jcv_serial_pop8(st);
    z80ctx.d_ = jcv_serial_pop8(st);
    z80ctx.e_ = jcv_serial_pop8(st);
    z80ctx.h_ = jcv_serial_pop8(st);
    z80ctx.l_ = jcv_serial_pop8(st);
    z80ctx.i  = jcv_serial_pop8(st);
    z80ctx.r  = jcv_serial_pop8(st);
    z80ctx.iff_delay = jcv_serial_pop8(st);
    z80ctx.interrupt_mode = jcv_serial_pop8(st);
    z80ctx.irq_data = jcv_serial_pop8(st);
    z80ctx.iff1 = jcv_serial_pop8(st);
    z80ctx.iff2 = jcv_serial_pop8(st);
    z80ctx.halted = jcv_serial_pop8(st);
    z80ctx.irq_pending = jcv_serial_pop8(st);
    z80ctx.nmi_pending = jcv_serial_pop8(st);
}

// Export the Z80's state
void jcv_z80_state_save(uint8_t *st) {
    jcv_serial_push16(st, z80ctx.pc);
    jcv_serial_push16(st, z80ctx.sp);
    jcv_serial_push16(st, z80ctx.ix);
    jcv_serial_push16(st, z80ctx.iy);
    jcv_serial_push16(st, z80ctx.mem_ptr);
    jcv_serial_push8(st, z80ctx.a);
    jcv_serial_push8(st, z80ctx.f);
    jcv_serial_push8(st, z80ctx.b);
    jcv_serial_push8(st, z80ctx.c);
    jcv_serial_push8(st, z80ctx.d);
    jcv_serial_push8(st, z80ctx.e);
    jcv_serial_push8(st, z80ctx.h);
    jcv_serial_push8(st, z80ctx.l);
    jcv_serial_push8(st, z80ctx.a_);
    jcv_serial_push8(st, z80ctx.f_);
    jcv_serial_push8(st, z80ctx.b_);
    jcv_serial_push8(st, z80ctx.c_);
    jcv_serial_push8(st, z80ctx.d_);
    jcv_serial_push8(st, z80ctx.e_);
    jcv_serial_push8(st, z80ctx.h_);
    jcv_serial_push8(st, z80ctx.l_);
    jcv_serial_push8(st, z80ctx.i);
    jcv_serial_push8(st, z80ctx.r);
    jcv_serial_push8(st, z80ctx.iff_delay);
    jcv_serial_push8(st, z80ctx.interrupt_mode);
    jcv_serial_push8(st, z80ctx.irq_data);
    jcv_serial_push8(st, z80ctx.iff1);
    jcv_serial_push8(st, z80ctx.iff2);
    jcv_serial_push8(st, z80ctx.halted);
    jcv_serial_push8(st, z80ctx.irq_pending);
    jcv_serial_push8(st, z80ctx.nmi_pending);
}
