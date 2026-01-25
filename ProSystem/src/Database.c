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
// Database.c
// ----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "Database.h"

bool cart_in_db = false;
bool database_enabled = true;
const char *database_filename;

bool database_Load(const char *digest) {
    cart_in_db = false;
    
    if (database_enabled) {
        FILE* file = fopen(database_filename, "r");
        if (file == NULL) {
            return false;
        }
        
        // max count of items in the database
        static int count = 17;
        
        char buffer[256];
        while (fgets(buffer, 256, file) != NULL) {
            if (strstr(buffer, digest)) {
                cart_in_db = true;
                char entry[count][256];
                for (int index = 0; index < count; ++index) {
                    char *buf = fgets(buffer, 256, file);
                    buffer[strcspn(buf, "\r\n")] = 0;
                    snprintf(entry[index], sizeof(entry[index]), "%s", buffer);
                }
                
                cartridge_type = (uint8_t)atoi(strchr(entry[1], '=') + 1);
                
                cartridge_pokey = (uint8_t)atoi(strchr(entry[2], '=') + 1);
                
                cartridge_controller[0] = (uint8_t)atoi(strchr(entry[3], '=') + 1);
                cartridge_controller[1] = (uint8_t)atoi(strchr(entry[4], '=') + 1);
                
                cartridge_region = (uint8_t)atoi(strchr(entry[5], '=') + 1);
                
                cartridge_flags = (uint32_t)atoi(strchr(entry[6], '=') + 1);
                
                // Optionally load the lightgun crosshair offsets, hblank, dual analog
                for (int index = 7; index < 11; ++index) {
                    if (strstr(entry[index], "crossx")) {
                        cartridge_crosshair_x = (int)atoi(strchr(entry[index], '=') + 1);
                    }
                    
                    if (strstr(entry[index], "crossy")) {
                        cartridge_crosshair_y = (int)atoi(strchr(entry[index], '=') + 1);
                    }
                    
                    if (strstr(entry[index], "hblank")) {
                        cartridge_hblank = (int)atoi(strchr(entry[index], '=') + 1);
                    }
                    
                    if (strstr(entry[index], "dualanalog")) {
                        cartridge_dualanalog = (uint8_t)atoi(strchr(entry[index], '=') + 1);
                    }
                    
                    if (strstr(entry[index], "pokey450")) {
                        cartridge_pokey450 = (uint8_t)atoi(strchr(entry[index], '=') + 1);
                        if (cartridge_pokey450) {
                            cartridge_pokey = 1;
                        }
                    }
                    
                    if (strstr(entry[index], "disablebios")) {
                        cartridge_disable_bios = (uint8_t)atoi(strchr(entry[index], '=') + 1);
                    }
                    
                    if (strstr(entry[index], "leftswitch")) {
                        cartridge_left_switch = (uint8_t)atoi(strchr(entry[index], '=') + 1);
                    }
                    
                    if (strstr(entry[index], "rightswitch")) {
                        cartridge_right_switch = (uint8_t)atoi(strchr(entry[index], '=') + 1);
                    }
                    
                    if (strstr(entry[index], "swapbuttons")) {
                        cartridge_swap_buttons = (uint8_t)atoi(strchr(entry[index], '=') + 1);
                    }
                    
                    if (strstr(entry[index], "hsc")) {
                        cartridge_hsc_enabled = (uint8_t)atoi(strchr(entry[index], '=') + 1);
                    }
                }
                
                break;
            }
        }
        fclose(file);
    }
    return true;
}
