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
// Riot.h
// ----------------------------------------------------------------------------
#ifndef RIOT_H
#define RIOT_H

#include "Equates.h"
#include "Memory.h"

extern void riot_Reset(void);
extern void riot_SetInput(const uint8_t* input);
extern void riot_SetDRA(uint8_t data);
extern void riot_SetDRB(uint8_t data);
extern void riot_SetTimer(uint16_t timer, uint8_t intervals);
extern void riot_UpdateTimer(uint8_t cycles);
extern bool riot_timing;
extern uint16_t riot_timer;
extern uint8_t riot_intervals;
extern uint8_t riot_dra;
extern uint8_t riot_drb;
extern uint16_t riot_clocks;

#endif
