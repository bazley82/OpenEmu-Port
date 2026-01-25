#ifndef __E8910_H
#define __E8910_H

#include <stdint.h>

void e8910_init_sound();
void e8910_done_sound();
void e8910_write(int r, int v);
void e8910_callback(uint8_t *stream, int length);

#endif
