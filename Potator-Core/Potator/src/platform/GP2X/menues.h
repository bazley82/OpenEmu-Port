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

#ifndef __MENUES_H__
#define __MENUES_H__

#include <dirent.h> 
#include <sys/types.h> 
#include <sys/param.h> 
#include <sys/stat.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "types.h"
#include "minimal.h"

extern unsigned short *screen16;

// Resume
#define RESIZE() \
	if(currentConfig.videoMode==2) \
		gp2x_video_RGB_setscaling(160, 160); \
	else \
		gp2x_video_RGB_setscaling(320, 240)

// Clear
#define textClear() \
		gp2x_memset(screen16, 0, 320*240*2); \
		gp2x_video_RGB_flip(0); \
		gp2x_memset(screen16, 0, 320*240*2); \
		gp2x_video_RGB_flip(0)
#define strcasestr(x, y) (strcasecmp(&x[strlen(x) - strlen(y)], y))
// Directory Constants
typedef enum {FT_NONE, FT_FILE, FT_DIR} FILE_TYPE;
		
typedef struct
{
	char fName[256];
	FILE_TYPE fType;
	BOOL isRunable;
		
} FileEntry, *pFileEntry;

extern FileEntry FileList[1024];
extern u32 fileCounter;
extern char *romname;

typedef struct {
	char lastRomDir[MAXPATHLEN];
	unsigned char videoMode;
	unsigned char show_fps;
	unsigned char enable_sound;
	unsigned short SoundRate;
	unsigned char Frameskip;
	unsigned char CPUclock;
	unsigned char volume;
} currentConfig_t;

extern currentConfig_t currentConfig;

void handleFileMenu(void);
void handleMainMenu(void);
void handleOptionsMenu(void);

void getRunDir(void);
void emu_ReadConfig(void);
void emu_WriteConfig(void);

#endif
