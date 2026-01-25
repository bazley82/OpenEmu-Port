/*
    This file is part of CrabEmu.

    Copyright (C) 2012, 2013 Lawrence Sebald

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

#include "nesapu.h"
#include "nes_apu.h"
#include "sound.h"

static apu_t *apu;

void nes_apu_write(uint16 addr, uint8 data) {
    apu_write(addr, data);
}

uint8 nes_apu_read(uint16 addr) {
    return apu_read(addr);
}

#ifndef _arch_dreamcast
void nes_apu_execute(int cycles __UNUSED__) {
    static int16 sbuf[735];

    apu_process(sbuf, 735);
    sound_update_buffer(sbuf, 735 << 1);
}
#else
void nes_apu_execute(int cycles __UNUSED__) {
    static int16 sbuf[735 << 1];
    static int frame = 0, samples = 0;

    apu_process(sbuf + samples, 735);
    frame ^= 1;
    samples += 735;

    if(!frame) {
        sound_update_buffer_noint(sbuf, NULL, NULL, 735 << 2);
        samples = 0;
    }
}
#endif

int nes_apu_init(void) {
    apu = apu_create(44100, 60, 16, FALSE);
    if(apu)
        return 0;

    return -1;
}

void nes_apu_shutdown(void) {
    apu_destroy(apu);
}

void nes_apu_reset(void) {
    apu_reset();
}

int nes_apu_write_context(FILE *fp) {
    uint8 data[4];
    uint8 regs[24];

    data[0] = 'N';
    data[1] = 'A';
    data[2] = 'P';
    data[3] = 'U';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(40, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    /* Copy in the registers */
    regs[0] = apu->rectangle[0].regs[0];
    regs[1] = apu->rectangle[0].regs[1];
    regs[2] = apu->rectangle[0].regs[2];
    regs[3] = apu->rectangle[0].regs[3];
    regs[4] = apu->rectangle[1].regs[0];
    regs[5] = apu->rectangle[1].regs[1];
    regs[6] = apu->rectangle[1].regs[2];
    regs[7] = apu->rectangle[1].regs[3];
    regs[8] = apu->triangle.regs[0];
    regs[9] = 0;
    regs[10] = apu->triangle.regs[1];
    regs[11] = apu->triangle.regs[2];
    regs[12] = apu->noise.regs[0];
    regs[13] = 0;
    regs[14] = apu->noise.regs[1];
    regs[15] = apu->noise.regs[2];
    regs[16] = apu->dmc.regs[0];
    regs[17] = apu->dmc.regs[1];
    regs[18] = apu->dmc.regs[2];
    regs[19] = apu->dmc.regs[3];
    regs[20] = 0;
    regs[21] = apu->enable_reg;
    regs[22] = 0;
    regs[23] = 0;
    fwrite(regs, 1, 24, fp);

    return 0;
}

int nes_apu_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 40)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Read in the registers */
    apu_regwrite(APU_WRA0, buf[16]);
    apu_regwrite(APU_WRA1, buf[17]);
    apu_regwrite(APU_WRA2, buf[18]);
    apu_regwrite(APU_WRA3, buf[19]);
    apu_regwrite(APU_WRB0, buf[20]);
    apu_regwrite(APU_WRB1, buf[21]);
    apu_regwrite(APU_WRB2, buf[22]);
    apu_regwrite(APU_WRB3, buf[23]);
    apu_regwrite(APU_WRC0, buf[24]);
    apu_regwrite(APU_WRC2, buf[26]);
    apu_regwrite(APU_WRC3, buf[27]);
    apu_regwrite(APU_WRD0, buf[28]);
    apu_regwrite(APU_WRD2, buf[30]);
    apu_regwrite(APU_WRD3, buf[31]);
    apu_regwrite(APU_WRE0, buf[32]);
    apu_regwrite(APU_WRE1, buf[33]);
    apu_regwrite(APU_WRE2, buf[34]);
    apu_regwrite(APU_WRE3, buf[35]);
    apu_regwrite(APU_SMASK, buf[37]);

    return 0;
}
