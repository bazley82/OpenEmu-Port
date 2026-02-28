#ifndef __IO_H__
#define __IO_H__

#include "types.h"

void io_init(void);
void io_reset(void);
void io_done(void);
void io_write(uint32 addr, uint8 data);
uint8 io_read(uint32 addr);

#endif
