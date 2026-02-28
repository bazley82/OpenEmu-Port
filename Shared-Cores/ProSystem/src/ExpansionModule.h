#ifndef EXPANSIONMODULE_H
#define EXPANSIONMODULE_H

#include "Cartridge.h"
#include "Memory.h"

#define XM_RAM_SIZE 0x20000

extern uint8_t xm_ram[XM_RAM_SIZE];
extern bool xm_pokey_enabled;
extern bool xm_mem_enabled;
extern uint8_t xm_reg;
extern uint8_t xm_bank;

void xm_Reset(void);
uint8_t xm_Read(uint16_t address);
void xm_Write(uint16_t address, uint8_t data);
#endif
