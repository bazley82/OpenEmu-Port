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

#ifndef SCALE2X_H
#define SCALE2X_H

#include "CrabEmu.h"

CLINKAGE

extern void scale2x_scale(const pixel_t *inbuf, pixel_t *outbuf, int width,
                          int height);

ENDCLINK

#endif /* SCALE2X_H */
