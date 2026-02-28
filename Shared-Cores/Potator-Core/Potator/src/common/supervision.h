////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __SUPERVISION_H__
#define __SUPERVISION_H__

#if defined __cplusplus
extern "C" {
#endif

#include "log.h"
#include "types.h"
#include "memory.h"
#include "version.h"
#include "io.h"
#include "gpu.h"
#include "timer.h"
#include "controls.h"
#include "memorymap.h"
#include "interrupts.h"

#ifdef GP2X
#include "menues.h"
#endif

#ifndef OPENEMU
#include ".\\m6502\\M6502.h"
#else
#include "./M6502/M6502.h"
#endif

typedef enum {
    COLOUR_SCHEME_DEFAULT,
    COLOUR_SCHEME_AMBER,
    COLOUR_SCHEME_GREEN,
    COLOUR_SCHEME_BLUE,
    COLOUR_SCHEME_COUNT
} COLOR_SCHEME;

extern void supervision_init(void);
extern void supervision_reset(void);
extern void supervision_reset_handler(void);
extern void supervision_done(void);
extern void supervision_exec(int16 *backbuffer, BOOL bRender);
extern void supervision_exec2(int16 *backbuffer, BOOL bRender);
extern void supervision_exec3(int16 *backbuffer, BOOL bRender);
extern void supervision_exec_fast(int16 *backbuffer, BOOL bRender);
extern BOOL supervision_load(uint8 *rom, uint32 romSize);
extern BOOL supervision_update_input(void);
extern void supervision_set_colour_scheme(COLOR_SCHEME ws_colourScheme);
extern M6502	*supervision_get6502regs(void);
extern void supervision_turnSound(BOOL bOn);

extern int	sv_loadState(const char *statepath, int id);
extern int	sv_saveState(const char *statepath, int id);
#endif

#if defined __cplusplus
};
#endif
