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
// Bios.c
// ----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "Bios.h"

bool bios_enabled = false;
char bios_filename[256];

static uint8_t* bios_data = NULL;
static uint16_t bios_size = 0;

bool bios_Load(const char *filename) {
    bios_Release();
    //logger_LogInfo("Opening bios file " + filename + ".", BIOS_SOURCE);
    
    if (bios_size == 0) {
        FILE* file = fopen(filename, "rb");
        if (file == NULL) {
            //logger_LogError("Failed to open the bios file " + filename + " for reading.", BIOS_SOURCE);
            return false;
        }
        
        if (fseek(file, 0L, SEEK_END)) {
            fclose(file);
            //logger_LogError("Failed to find the end of the bios file.", BIOS_SOURCE);
            return false;
        }
        
        bios_size = ftell(file);
        if (fseek(file, 0L, SEEK_SET)) {
            fclose(file);
            //logger_LogError("Failed to find the size of the bios file.", BIOS_SOURCE);
            return false;
        }
        
        bios_data = (uint8_t*)malloc(bios_size * sizeof(uint8_t));
        if (fread(bios_data, 1, bios_size, file) != bios_size && ferror(file)) {
            fclose(file);
            //logger_LogError("Failed to read the bios data.", BIOS_SOURCE);
            bios_Release( );
            return false;
        }
        
        fclose(file);
    }
    
    snprintf(bios_filename, sizeof(bios_filename), "%s", filename);
    return true;
}

bool bios_IsLoaded(void) {
    return (bios_data != NULL) ? true : false;
}

void bios_Release(void) {
    if (bios_data) {
        free(bios_data);
        bios_size = 0;
        bios_data = NULL;
    }
}

void bios_Store(void) {
    if (bios_data != NULL && bios_enabled) {
        memory_WriteROM(65536 - bios_size, bios_size, bios_data);
    }
}
