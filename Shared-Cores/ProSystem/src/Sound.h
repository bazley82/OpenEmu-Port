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
// Sound.h
// ----------------------------------------------------------------------------
#ifndef SOUND_H
#define SOUND_H

#include "ProSystem.h"
#include "Tia.h"
#include "Pokey.h"
#include "Cartridge.h"

extern uint32_t sound_Store(uint8_t *out_buffer);
extern void sound_SetSampleRate(uint32_t rate);
extern uint32_t sound_GetSampleRate(void);

#endif
