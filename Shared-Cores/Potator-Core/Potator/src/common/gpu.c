////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
#include "gpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef GP2X
#include "minimal.h"
#endif
#ifdef NDS
#include <nds.h>
#endif

static uint16	*supervision_palette;
static uint8    gpu_regs[4];
#ifdef NDS
#define RGB555(R,G,B) ((((int)(B))<<10)|(((int)(G))<<5)|(((int)(R)))|BIT(15))
#else
#define RGB555(R,G,B) ((((int)(B))<<10)|(((int)(G))<<5)|(((int)(R))))
#endif

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void gpu_init(void)
{
	#ifdef DEBUG
	printf("Gpu Init\n");
	#endif
	//fprintf(log_get(), "gpu: init\n");
	memory_malloc_secure((void**)&supervision_palette,  4*sizeof(int16), "Palette");
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
void gpu_done(void)
{
	//fprintf(log_get(), "gpu: done\n");
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
void gpu_reset(void)
{
	#ifdef DEBUG
	printf("Gpu Reset\n");
	#endif

#ifdef GP2X
	supervision_palette[3] = gp2x_video_RGB_color16(0,0,0);
	supervision_palette[2] = gp2x_video_RGB_color16(85,85,85);
	supervision_palette[1] = gp2x_video_RGB_color16(170,170,170);
	supervision_palette[0] = gp2x_video_RGB_color16(255,255,255);
#endif
#ifdef NDS
	supervision_palette[3] = RGB555(0,0,0);
	supervision_palette[2] = RGB555(10,10,10);
	supervision_palette[1] = RGB555(20,20,20);
	supervision_palette[0] = RGB555(30,30,30);
#endif 
#ifdef _WIN_
    supervision_palette[3] = RGB555(0,0,0);
	supervision_palette[2] = RGB555(10,10,10);
	supervision_palette[1] = RGB555(20,20,20);
	supervision_palette[0] = RGB555(30,30,30);
#endif
#ifdef OPENEMU
    supervision_palette[3] = RGB555(0,0,0);
    supervision_palette[2] = RGB555(10,10,10);
    supervision_palette[1] = RGB555(20,20,20);
    supervision_palette[0] = RGB555(30,30,30);
#endif

	memset(gpu_regs, 0, 4);
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
void gpu_set_colour_scheme(int colourScheme)
{
	#ifndef DEBUG
	printf("Gpu Set Color Scheme\n");
	#endif

	float greenf=1;
	float bluef=1;
	float redf=1;

	switch (colourScheme)
	{
	case COLOUR_SCHEME_DEFAULT:
		break;
	case COLOUR_SCHEME_AMBER:
		greenf=0.61f;
		bluef=0.00f;
		redf=1.00f;
		break;
	case COLOUR_SCHEME_GREEN:
		greenf=0.90f;
		bluef=0.20f;
		redf=0.20f;
		break;
	case COLOUR_SCHEME_BLUE:
		greenf=0.30f;
		bluef=0.75f;
		redf=0.30f;
		break;
	default: 
		colourScheme=0; 
		break;
	}
#ifdef GP2X
	supervision_palette[3] = gp2x_video_RGB_color16(0*redf,0*greenf,0*bluef);
	supervision_palette[2] = gp2x_video_RGB_color16(85*redf,85*greenf,85*bluef);
	supervision_palette[1] = gp2x_video_RGB_color16(170*redf,170*greenf,170*bluef);
	supervision_palette[0] = gp2x_video_RGB_color16(255*redf,255*greenf,255*bluef);
#endif
#ifdef NDS
	supervision_palette[3] = RGB555(0*redf,0*greenf,0*bluef);
	supervision_palette[2] = RGB555(10*redf,10*greenf,10*bluef);
	supervision_palette[1] = RGB555(20*redf,20*greenf,20*bluef);
	supervision_palette[0] = RGB555(30*redf,30*greenf,30*bluef);
#endif
#ifdef _WIN_
	supervision_palette[3] = RGB555(0*redf,0*greenf,0*bluef);
	supervision_palette[2] = RGB555(10*redf,10*greenf,10*bluef);
	supervision_palette[1] = RGB555(20*redf,20*greenf,20*bluef);
	supervision_palette[0] = RGB555(30*redf,30*greenf,30*bluef);
#endif
#ifdef OPENEMU
    supervision_palette[3] = RGB555(0*redf,0*greenf,0*bluef);
    supervision_palette[2] = RGB555(10*redf,10*greenf,10*bluef);
    supervision_palette[1] = RGB555(20*redf,20*greenf,20*bluef);
    supervision_palette[0] = RGB555(30*redf,30*greenf,30*bluef);
#endif
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
void gpu_write(uint32 addr, uint8 data)
{
	gpu_regs[(addr&0x03)] = data;
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
uint8 gpu_read(uint32 addr)
{
	return(gpu_regs[(addr&0x03)]);
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
void gpu_render_scanline(uint32 scanline, int16 *backbuffer)
{
	uint8 *vram_line = &(memorymap_getUpperRamPointer())[(gpu_regs[2] >> 2) + (scanline*0x30)];
	uint8 x;

	for (x =0; x < 160; x += 4)
	{
		uint8 b = *(vram_line++);
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;
	}
}

void gpu_render_scanline_fast(uint32 scanline, int16 *backbuffer)
{
	uint8 *vram_line = &(memorymap_getUpperRamPointer())[(gpu_regs[2] / 4) + (scanline*0x30)];
	//uint32 x;

//unfolded for to gain speed
//	for (x = 0; x < 160; x += 4) 
//	{
		uint8 b = *(vram_line++);
  	
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;
  	
  	    b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
	    b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;
		
		b = *(vram_line++);
        backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;
		b = *(vram_line++);
  	
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;
  	
  	    b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
	    b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;  	
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];		
		backbuffer += 4;
		
		b = *(vram_line++);
        backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;
		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;		
		b = *(vram_line++);
		backbuffer[4] = supervision_palette[((b >> 8) & 0x03)];
		backbuffer[3] = supervision_palette[((b >> 6) & 0x03)];
		backbuffer[2] = supervision_palette[((b >> 4) & 0x03)];
		backbuffer[1] = supervision_palette[((b >> 2) & 0x03)];
		backbuffer[0] = supervision_palette[((b >> 0) & 0x03)];
		backbuffer += 4;
		
//	}
}

