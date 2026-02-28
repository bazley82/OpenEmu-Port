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
// ProSystem.h
// ----------------------------------------------------------------------------
#ifndef PRO_SYSTEM_H
#define PRO_SYSTEM_H

#include "Equates.h"
#include "Bios.h"
#include "Cartridge.h"
#include "Maria.h"
#include "Memory.h"
#include "Region.h"
#include "Riot.h"
#include "Sally.h"
#include "Tia.h"
#include "Pokey.h"
#include "ExpansionModule.h"

extern void prosystem_Reset(void);
extern void prosystem_ExecuteFrame(const uint8_t* input);
extern bool prosystem_Save(const char *filename);
extern bool prosystem_Load(const char *filename);
extern bool prosystem_Save_buffer(uint8_t *buffer);
extern bool prosystem_Load_buffer(const uint8_t *buffer);
extern void prosystem_Pause(bool pause);
extern void prosystem_Close(void);

// The number of cycles per scan line
#define CYCLES_PER_SCANLINE 454
// The number of cycles for HBLANK
#define HBLANK_CYCLES 136
// The number of cycles per scanline that the 7800 checks for a hit
#define LG_CYCLES_PER_SCANLINE 318
// The number of cycles indented (after HBLANK) prior to checking for a hit
#define LG_CYCLES_INDENT 52

extern bool prosystem_active;
extern bool prosystem_paused;
extern uint16_t prosystem_frequency;
extern uint8_t prosystem_frame;
extern uint16_t prosystem_scanlines;
extern uint32_t prosystem_cycles;
extern uint32_t prosystem_extra_cycles;

// The scanline that the lightgun shot occurred at
extern int lightgun_scanline;
// The cycle that the lightgun shot occurred at
extern float lightgun_cycle;
// Whether the lightgun is enabled for the current cartridge
//extern bool lightgun_enabled;

#endif
