/*
    This file is part of CrabEmu.

    Copyright (C) 2009 Lawrence Sebald

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

#ifndef SDSCTERMINAL_H
#define SDSCTERMINAL_H

#include "CrabEmu.h"

CLINKAGE

#ifdef ENABLE_SDSC_TERMINAL

#define SDSC_CTL_SUSPEND        1
#define SDSC_CTL_CLEAR          2
#define SDSC_CTL_SET_ATTRIBUTE  3
#define SDSC_CTL_MOVE_CURSOR    4

extern void sms_sdsc_data_write(uint8 data);
extern void sms_sdsc_ctl_write(uint8 data);

void gui_update_sdsc_terminal(char console[25][80 * 2]);

#endif /* ENABLE_SDSC_TERMINAL */

extern void sms_sdsc_reset(void);

ENDCLINK

#endif /* !SDSCTERMINAL_H */
