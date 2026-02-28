// ----------------------------------------------------------------------------
//   ___  ___  ___  ___       ___  ____  ___  _  _      __  ___
//  /__/ /__/ /  / /__  /__/ /__    /   /_   / |/ /      / / _
// /    / \  /__/ ___/ ___/ ___/   /   /__  /    /   ___/ /__/
//
// ----------------------------------------------------------------------------
// Copyright 2005 Greg Stanton
// Copyright 2020 Rupert Carmichael
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// ----------------------------------------------------------------------------
// PokeySound is Copyright(c) 1997 by Ron Fries
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of version 2 of the GNU Library General Public License
// as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
// General Public License for more details.
// To obtain a copy of the GNU Library General Public License, write to the
// Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Any permitted reproduction of these routines, in whole or in part, must
// bear this legend.
// ----------------------------------------------------------------------------
// Pokey.h
// ----------------------------------------------------------------------------
#ifndef POKEY_H
#define POKEY_H

#define POKEY_BUFFER_SIZE 624
#define POKEY_AUDF1 0x4000
#define POKEY_AUDC1 0x4001
#define POKEY_AUDF2 0x4002
#define POKEY_AUDC2 0x4003
#define POKEY_AUDF3 0x4004
#define POKEY_AUDC3 0x4005
#define POKEY_AUDF4 0x4006
#define POKEY_AUDC4 0x4007
#define POKEY_AUDCTL 0x4008
#define POKEY_STIMER 0x4009
#define POKEY_SKRES 0x400a
#define POKEY_POTGO 0x400b
#define POKEY_SEROUT 0x400d
#define POKEY_IRQEN 0x400e
#define POKEY_SKCTLS 0x400f

#define POKEY_POT0 0x4000
#define POKEY_POT1 0x4001
#define POKEY_POT2 0x4002
#define POKEY_POT3 0x4003
#define POKEY_POT4 0x4004
#define POKEY_POT5 0x4005
#define POKEY_POT6 0x4006
#define POKEY_POT7 0x4007
#define POKEY_ALLPOT 0x4008
#define POKEY_KBCODE 0x4009
#define POKEY_RANDOM 0x400a
#define POKEY_SERIN 0x400d
#define POKEY_IRQST 0x400e
#define POKEY_SKSTAT 0x400f

extern void pokey_Reset(void);
extern void pokey_SetRegister(uint16_t address, uint8_t value);
extern uint8_t pokey_GetRegister(uint16_t address);
extern void pokey_Process(uint32_t length);
extern void pokey_Clear(void);
extern uint8_t pokey_buffer[POKEY_BUFFER_SIZE];
extern uint32_t pokey_size;

extern void pokey_Frame(void);
extern void pokey_Scanline(void);
extern void pokey_setSampleRate(uint32_t rate);

#endif
