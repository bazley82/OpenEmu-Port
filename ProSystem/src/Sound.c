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
// Sound.c
// ----------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "Sound.h"

#define MAX_BUFFER_SIZE 8192

static uint32_t nSamplesPerSec = 48000;

static void sound_Resample(const uint8_t *source, uint8_t *target, int length) {
    int measurement = nSamplesPerSec;
    int sourceIndex = 0;
    int targetIndex = 0;
    int max = ((prosystem_frequency * prosystem_scanlines) << 1);
    
    while (targetIndex < length) {
        if (measurement >= max) {
            target[targetIndex++] = source[sourceIndex];
            measurement -= max;
        }
        else {
            ++sourceIndex;
            measurement += nSamplesPerSec;
        }
    }
}

uint32_t sound_Store(uint8_t *out_buffer) {
    memset(out_buffer, 0, MAX_BUFFER_SIZE);
    uint32_t length = nSamplesPerSec / prosystem_frequency;
    sound_Resample(tia_buffer, out_buffer, length);
    tia_Clear();
    
    // Ballblazer, Commando, various homebrew and hacks
    if(cartridge_pokey || xm_pokey_enabled) {
        uint8_t pokeySample[MAX_BUFFER_SIZE];
        memset(pokeySample, 0, MAX_BUFFER_SIZE);
        sound_Resample(pokey_buffer, pokeySample, length);
        
        for(uint32_t index = 0; index < length; ++index) {
            uint32_t sound = out_buffer[index] + pokeySample[index];
            out_buffer[index] = sound >> 1;
        }
    }
    else {
        for(uint32_t index = 0; index < length; ++index) {
            out_buffer[index] = out_buffer[index] * 0.75; //>> 1;
        }
    }
    pokey_Clear();
    
    return length;
}

void sound_SetSampleRate(uint32_t rate) {
    nSamplesPerSec = rate;
}

uint32_t sound_GetSampleRate(void) {
    return nSamplesPerSec;
}
