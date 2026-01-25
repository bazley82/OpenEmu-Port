#ifndef __CONTROLS_H__
#define __CONTROLS_H__

#include "supervision.h"

#ifdef OPENEMU
extern uint8 controls_state;
#endif

void controls_init();
void controls_done();
void controls_reset();
void controls_write(uint32 addr, uint8 data);
uint8 controls_read(uint32 addr);
BOOL controls_update(void);
void controls_state_write(uint8 type, uint8 data);

#endif
