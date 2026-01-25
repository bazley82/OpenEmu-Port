#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include "supervision.h"

void interrupts_init();
void interrupts_reset();
void interrupts_done();
void interrupts_write(uint32 Addr, uint8 data);
uint8 interrupts_read(uint32 Addr);
void interrupts_exec(uint32 cycles);
void interrupts_nmi();
void interrupts_irq();

#endif
