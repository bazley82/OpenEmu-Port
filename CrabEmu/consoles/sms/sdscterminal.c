/*
    This file is part of CrabEmu.

    Copyright (C) 2009, 2014 Lawrence Sebald

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

/* Fair warning to anyone reading this: This file has some really ugly code in
   it. */

#include <string.h>
#include "sms.h"
#include "smsz80.h"
#include "sdscterminal.h"
#include "smsvdp.h"
#include "smsmem.h"

#ifdef ENABLE_SDSC_TERMINAL

extern uint8 *sms_read_map[256];

static char console[25][80 * 2];
static int cur_x = 0;
static int cur_y = 0;
static char cur_attr = 0x0F;

static int cur_data_state = 0;
static int cur_data_width = 0;
static uint8 cur_data_format = 0;
static uint8 cur_data_type1 = 0;
static uint8 cur_data_type2 = 0;
static uint8 cur_data_param1 = 0;
static uint8 cur_data_param2 = 0;

static int ctl_state = 0;

static void sdsc_console_clear(void) {
    int i, j;

    for(i = 0; i < 25; ++i) {
        for(j = 0; j < 80; ++j) {
            console[i][j << 1] = cur_attr;
            console[i][(j << 1) + 1] = ' ';
        }
    }

    cur_x = 0;
    cur_y = 0;

    gui_update_sdsc_terminal(console);
}

static void sdsc_increment_y(void) {
    int i;

    ++cur_y;

    if(cur_y == 25) {
        /* Shift data up one row. */
        for(i = 0; i < 24; ++i) {
            memcpy(&console[i][0], &console[i + 1][0], 80 << 1);
        }

        cur_y = 24;
    }
}

static void sdsc_increment_x(void) {
    ++cur_x;

    if(cur_x == 80) {
        cur_x = 0;

        sdsc_increment_y();
    }
}

static void sdsc_write_format_num(void) {
    char str[257];
    int num = 0, i, bits = 8;
    uint16 addr;

    if(cur_data_type1 == 'm') {
        if(cur_data_type2 == 'b') {
            num = sms_read_map[cur_data_param2][cur_data_param1];
        }
        else if(cur_data_type2 == 'w') {
            num = sms_read_map[cur_data_param2][cur_data_param1++];

            if(cur_data_param1 == 0)
                ++cur_data_param2;

            num |= sms_read_map[cur_data_param2][cur_data_param1] << 8;
            bits = 16;
        }
    }
    else if(cur_data_type1 == 'v') {
        addr = (cur_data_param1 | (cur_data_param2 << 8)) & 0x3FFF;

        if(cur_data_type2 == 'b') {
            num = smsvdp.vram[addr];
        }
        else if(cur_data_type2 == 'w') {
            num = smsvdp.vram[addr++];

            if(addr == 0x4000)
                addr = 0;

            num |= smsvdp.vram[addr] << 8;
            bits = 16;
        }
        else if(cur_data_type2 == 'r') {
            cur_data_param1 &= 0x2F;

            if(cur_data_param1 < 0x10) {
                num = smsvdp.regs[cur_data_param1];
            }
            else if(sms_cons._base.console_type == CONSOLE_SMS) {
                num = smsvdp.cram[cur_data_param1 - 0x10];
            }
            else if(sms_cons._base.console_type == CONSOLE_GG) {
                addr = (cur_data_param1 - 0x10) << 1;
                num = smsvdp.cram[addr] | (smsvdp.cram[addr + 1] << 8);
                bits = 16;
            }
        }
    }
    else if(cur_data_type1 == 'p' && cur_data_type2 == 'r') {
        num = sms_z80_read_reg(cur_data_param1);

        if(cur_data_param1 >= 0x08 && cur_data_param1 != 0x10 &&
           cur_data_param1 != 0x11) {
            bits = 16;
        }
    }

    if(cur_data_format == 'd') {
        /* Sign extend the number... */
        if(bits == 16 && (num & 0x8000)) {
            num |= 0xFFFF0000;
        }
        else if(bits == 8 && (num & 0x80)) {
            num |= 0xFFFFFF00;
        }

        sprintf(str, "%d", num);
    }
    else if(cur_data_format == 'u') {
        sprintf(str, "%u", num);
    }
    else if(cur_data_format == 'x') {
        sprintf(str, "%x", num);
    }
    else if(cur_data_format == 'X') {
        sprintf(str, "%X", num);
    }
    else {
        for(i = 0; i < bits; ++i) {
            str[bits - i - 1] = (num & (1 << i)) ? '1' : '0';
        }

        /* XXXX: Should shift off any 0's from the front... */

        str[bits] = 0;
    }

    num = strlen(str);

    for(i = 0; i < num; ++i) {
        console[cur_y][cur_x << 1] = cur_attr;
        console[cur_y][(cur_x << 1) + 1] = str[i];
        sdsc_increment_x();
    }
}

static void sdsc_write_format_char(void) {
    char letter = ' ';

    if(cur_data_type1 == 'm' && cur_data_type2 == 'b') {
        letter = sms_read_map[cur_data_param2][cur_data_param1];
    }
    else if(cur_data_type1 == 'v' && cur_data_type2 == 'b') {
        int addr = (cur_data_param1 | (cur_data_param2 << 8)) & 0x3FFF;

        letter = smsvdp.vram[addr];
    }

    console[cur_y][cur_x << 1] = cur_attr;
    console[cur_y][(cur_x << 1) + 1] = letter;
    sdsc_increment_x();
}

static void sdsc_write_format_str(void) {
    char str[257];
    int i, end = 0;
    char letter;

    if(cur_data_type1 == 'm' && cur_data_type2 == 'b') {
        for(i = 0; i < 256 && !end; ++i) {
            letter = sms_read_map[cur_data_param2][cur_data_param1];
            ++cur_data_param1;

            if(cur_data_param1 == 0) {
                ++cur_data_param2;
            }

            str[i] = letter;
            end = !letter;
        }
    }
    else if(cur_data_type1 == 'v' && cur_data_type2 == 'b') {
        int addr = (cur_data_param1 | (cur_data_param2 << 8)) & 0x3FFF;
        
        for(i = 0; i < 256 && !end; ++i) {
            letter = smsvdp.vram[addr++];
            
            if(addr == 0x4000) {
                addr = 0;
            }
            
            str[i] = letter;
            end = !letter;
        }
    }

    str[256] = 0;
    end = strlen(str);

    for(i = 0; i < end; ++i) {
        if(str[i] == 0x0A) {
            sdsc_increment_y();
            cur_x = 0;
        }
        else {
            console[cur_y][cur_x << 1] = cur_attr;
            console[cur_y][(cur_x << 1) + 1] = str[i];
            sdsc_increment_x();
        }
    }
}

static void sdsc_write_format(void) {
    if(cur_data_format == 'd' || cur_data_format == 'u' ||
       cur_data_format == 'x' || cur_data_format == 'X' ||
       cur_data_format == 'b') {
        sdsc_write_format_num();
    }
    else if(cur_data_format == 'a') {
        sdsc_write_format_char();
    }
    else if(cur_data_format == 's') {
        sdsc_write_format_str();
    }

    /* Reset variables. */
    cur_data_state = 0;
    cur_data_width = 0;
    cur_data_format = 0;
    cur_data_type1 = 0;
    cur_data_type2 = 0;
    cur_data_param1 = 0;
    cur_data_param2 = 0;

    gui_update_sdsc_terminal(console);
}

void sms_sdsc_data_write(uint8 data) {
    switch(cur_data_state) {
        case 0:
            if(data == 0x0A) {
                cur_x = 0;
                sdsc_increment_y();
                gui_update_sdsc_terminal(console);
            }
            else if(data == 0x0D) {
                cur_x = 0;
            }
            else if(data != '%') {
                console[cur_y][cur_x << 1] = cur_attr;
                console[cur_y][(cur_x << 1) + 1] = (char)data;
                sdsc_increment_x();
                gui_update_sdsc_terminal(console);
            }
            else {
                ++cur_data_state;
                cur_data_width = 0;
            }
            break;

        case 1:
            if(data == '%') {
                console[cur_y][cur_x << 1] = cur_attr;
                console[cur_y][(cur_x << 1) + 1] = (char)data;
                cur_data_state = 0;
                sdsc_increment_x();
                gui_update_sdsc_terminal(console);
            }
            else if(data == 'd' || data == 'u' || data == 'x' || data == 'X' ||
                    data == 'b' || data == 'a' || data == 's') {
                ++cur_data_state;
                cur_data_format = data;
            }
            else {
                cur_data_width *= 10;
                cur_data_width += data - '0';
            }
            break;

        case 2:
            cur_data_type1 = data;
            ++cur_data_state;
            break;

        case 3:
            cur_data_type2 = data;
            ++cur_data_state;
            break;

        case 4:
            cur_data_param1 = data;
            if((cur_data_type1 == 'm' || cur_data_type1 == 'v') &&
               (cur_data_type2 == 'w' || cur_data_type2 == 'b')) {
                ++cur_data_state;
            }
            else if((cur_data_type1 == 'p' && cur_data_type2 == 'r') ||
                    (cur_data_type1 == 'v' && cur_data_type2 == 'r')) {
                sdsc_write_format();
            }
            else {
                /* There was some kind of problem... punt. */
                ++cur_data_state;
            }
            break;

        case 5:
            cur_data_param2 = data;
            sdsc_write_format();
            break;
    }

    data = 0;
}

void sms_sdsc_ctl_write(uint8 data) {
    if(ctl_state == 0) {
        switch(data) {
            case SDSC_CTL_SUSPEND:
                /* XXXX: What is this supposed to actually do? */
                break;
            case SDSC_CTL_CLEAR:
                sdsc_console_clear();
                break;
            case SDSC_CTL_SET_ATTRIBUTE:
                ctl_state = SDSC_CTL_SET_ATTRIBUTE;
                break;
            case SDSC_CTL_MOVE_CURSOR:
                ctl_state = SDSC_CTL_MOVE_CURSOR;
                break;
        }
    }
    else if(ctl_state == SDSC_CTL_SET_ATTRIBUTE) {
        cur_attr = data;
        ctl_state = 0;
    }
    else if(ctl_state == SDSC_CTL_MOVE_CURSOR) {
        cur_x = data % 80;
        ctl_state |= 0x8000000;
    }
    else if(ctl_state == (SDSC_CTL_MOVE_CURSOR | 0x8000000)) {
        cur_y = data % 25;
        ctl_state = 0;
    }
}

void sms_sdsc_reset(void) {
    ctl_state = 0;
    cur_attr = 0x0F;
    cur_data_state = 0;

    sdsc_console_clear();
}

#else

void sms_sdsc_reset(void) {
    /* Nothing. */
}

#endif /* ENABLE_SDSC_TERMINAL */
