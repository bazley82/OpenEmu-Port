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
#include "supervision.h"
#include "memorymap.h"
#include "sound.h"
#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef GP2X
#include "menues.h"
#include "minimal.h"
#endif
#ifdef NDS
#include <nds.h>
#endif

static M6502	m6502_registers;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
byte Loop6502(register M6502 *R)
{
	return(INT_QUIT);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void supervision_init(void)
{
	//fprintf(log_get(), "supervision: init\n");
	#ifndef DEBUG
	//iprintf("supervision: init\n");
	#endif

	memorymap_init();
	io_init();
	gpu_init();
	timer_init();
	controls_init();
	sound_init();
	interrupts_init();
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
BOOL supervision_load(uint8 *rom, uint32 romSize)
{
	//uint32 supervision_programRomSize;
	//uint8 *supervision_programRom = memorymap_rom_load(szPath, &supervision_programRomSize);
	#ifdef DEBUG
	//iprintf("supervision: load\n");
	#endif

	memorymap_load(rom, romSize);
	supervision_reset();

	return(TRUE);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void supervision_reset(void)
{
	//fprintf(log_get(), "supervision: reset\n");


	memorymap_reset();
	io_reset();
	gpu_reset();
	timer_reset();
	controls_reset();
	sound_reset();
	interrupts_reset();

	Reset6502(&m6502_registers);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void supervision_reset_handler(void)
{
	//fprintf(log_get(), "supervision: reset handler\n");
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void supervision_done(void)
{
	//fprintf(log_get(), "supervision: done\n");
	memorymap_done();
	io_done();
	gpu_done();
	timer_done();
	controls_done();
	sound_done();
	interrupts_done();
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void supervision_set_colour_scheme(COLOR_SCHEME ws_colourScheme)
{
	gpu_set_colour_scheme(ws_colourScheme);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
M6502	*supervision_get6502regs(void)
{
	return(&m6502_registers);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
BOOL supervision_update_input(void)
{
	return(controls_update());
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void supervision_exec(int16 *backbuffer, BOOL bRender)
{
	uint32 supervision_scanline;

	for (supervision_scanline = 0; supervision_scanline < 160; supervision_scanline++)
	{
		m6502_registers.ICount = 512/2; //shouldnt really devide by two its just quicker :)
		timer_exec(m6502_registers.ICount);
#ifdef GP2X
		if(currentConfig.enable_sound) sound_exec(11025/160);
#else
		sound_exec(11025/160);
#endif
		Run6502(&m6502_registers);
#ifdef NDS
		gpu_render_scanline(supervision_scanline, backbuffer);
        printf("\n");
		backbuffer += 160+96;
#else
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
#endif
	}

	if (Rd6502(0x2026)&0x01)
        Int6502(supervision_get6502regs(), INT_NMI);
    printf("\n");
}

void supervision_exec2(int16 *backbuffer, BOOL bRender)
{
	uint32 supervision_scanline;

	for (supervision_scanline = 0; supervision_scanline < 160; supervision_scanline++)
	{
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
#ifdef GP2X
		if(currentConfig.enable_sound) sound_exec(11025/160);
#else
		sound_exec(11025/160);
#endif
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer+(80+40*320));
		backbuffer += 160*2;
	}

	if (Rd6502(0x2026)&0x01)
		Int6502(supervision_get6502regs(), INT_NMI);
}

void supervision_exec3(int16 *backbuffer, BOOL bRender)
{
	uint32 supervision_scanline;

	for (supervision_scanline = 0; supervision_scanline < 160; supervision_scanline++)
	{
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
#ifdef GP2X
		if(currentConfig.enable_sound) sound_exec(11025/160);
#else
		sound_exec(11025/160);
#endif
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160*2;
	}

	if (Rd6502(0x2026)&0x01)
		Int6502(supervision_get6502regs(), INT_NMI);
}

void supervision_exec_fast(int16 *backbuffer, BOOL bRender)
{
//	for (uint32 supervision_scanline = 0; supervision_scanline < 160; supervision_scanline+=4)
		uint32 supervision_scanline = 0;
	    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
			    m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);
		backbuffer += 160;
		supervision_scanline++;
		m6502_registers.ICount = 512;
		timer_exec(m6502_registers.ICount);
		sound_exec(22050/160);
		Run6502(&m6502_registers);
		gpu_render_scanline(supervision_scanline, backbuffer);

	

	if (Rd6502(0x2026)&0x01)
		interrupts_nmi();
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void supervision_turnSound(BOOL bOn)
{
	audio_turnSound(bOn);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

int	sv_loadState(const char *statepath, int id)
{
	FILE* fp;
	char newPath[256];

	strcpy(newPath,statepath);

#ifdef GP2X
	gp2x_printf(0,10,220,"newPath = %s",newPath);
	gp2x_video_RGB_flip(0);
#endif
#ifdef NDS
	iprintf("\nnewPath = %s",newPath);
#endif

	fp=fopen(newPath,"rb");

	if (fp) {
		fread(&m6502_registers, 1, sizeof(m6502_registers), fp);
		fread(memorymap_programRom, 1, sizeof(memorymap_programRom), fp);
		fread(memorymap_lowerRam, 1, 0x2000, fp);
		fread(memorymap_upperRam, 1, 0x2000, fp);
		fread(memorymap_lowerRomBank, 1, sizeof(memorymap_lowerRomBank), fp);
		fread(memorymap_upperRomBank, 1, sizeof(memorymap_upperRomBank), fp);
		fread(memorymap_regs, 1, 0x2000, fp);
		fclose(fp);
	}

#ifdef GP2X
	sleep(1);
#endif

	return(1);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

int	sv_saveState(const char *statepath, int id)
{
	FILE* fp;
	char newPath[256];

	strcpy(newPath,statepath);

#ifdef GP2X
	gp2x_printf(0,10,220,"newPath = %s",newPath);
	gp2x_video_RGB_flip(0);
#endif
#ifdef NDS
	iprintf("\nnewPath = %s",newPath);
#endif

	fp=fopen(newPath,"wb");

	if (fp) {
		fwrite(&m6502_registers, 1, sizeof(m6502_registers), fp);
		fwrite(memorymap_programRom, 1, sizeof(memorymap_programRom), fp);
		fwrite(memorymap_lowerRam, 1, 0x2000, fp);
		fwrite(memorymap_upperRam, 1, 0x2000, fp);
		fwrite(memorymap_lowerRomBank, 1, sizeof(memorymap_lowerRomBank), fp);
		fwrite(memorymap_upperRomBank, 1, sizeof(memorymap_upperRomBank), fp);
		fwrite(memorymap_regs, 1, 0x2000, fp);
		fflush(fp);
		fclose(fp);
#ifdef GP2X
		sync();
#endif
	}

#ifdef GP2X
	sleep(1);
#endif

	return(1);
}
