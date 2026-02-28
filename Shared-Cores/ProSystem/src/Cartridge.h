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
// Cartridge.h
// ----------------------------------------------------------------------------
#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#define CARTRIDGE_TYPE_NORMAL 0
#define CARTRIDGE_TYPE_SUPERCART 1
#define CARTRIDGE_TYPE_SUPERCART_LARGE 2
#define CARTRIDGE_TYPE_SUPERCART_RAM 3
#define CARTRIDGE_TYPE_SUPERCART_ROM 4
#define CARTRIDGE_TYPE_ABSOLUTE 5
#define CARTRIDGE_TYPE_ACTIVISION 6
#define CARTRIDGE_TYPE_NORMAL_RAM 7
#define CARTRIDGE_CONTROLLER_NONE 0
#define CARTRIDGE_CONTROLLER_JOYSTICK 1
#define CARTRIDGE_CONTROLLER_LIGHTGUN 2
#define CARTRIDGE_WSYNC_MASK 2
#define CARTRIDGE_CYCLE_STEALING_MASK 1

#define HBLANK_DEFAULT 34

#include "Equates.h"
#include "Memory.h"
#include "md5.h"
#include "Pokey.h"

extern bool cartridge_Load(const uint8_t* data, uint32_t size);
extern void cartridge_Store(void);
extern void cartridge_StoreBank(uint8_t bank);
extern void cartridge_Write(uint16_t address, uint8_t data);
extern bool cartridge_IsLoaded(void);
extern void cartridge_Release(void);
extern char cart_digest[33];
extern const char *cartridge_filename;
extern char cartridge_title[33];
extern uint8_t cartridge_type;
extern uint8_t cartridge_region;
extern bool cartridge_pokey;
extern bool cartridge_pokey450;
extern bool cartridge_xm;
extern uint8_t cartridge_controller[2];
extern uint8_t cartridge_bank;
extern uint32_t cartridge_flags;
extern bool cartridge_disable_bios;
extern uint8_t cartridge_left_switch;
extern uint8_t cartridge_right_switch;
extern bool cartridge_swap_buttons;
extern bool cartridge_hsc_enabled;

// The x offset for the lightgun crosshair (allows per cartridge adjustments)
extern int cartridge_crosshair_x;
// The y offset for the lightgun crosshair (allows per cartridge adjustments)
extern int cartridge_crosshair_y;
// The hblank prior to DMA
extern uint32_t cartridge_hblank;
// Whether the cartridge supports dual analog
extern bool cartridge_dualanalog;

#endif
