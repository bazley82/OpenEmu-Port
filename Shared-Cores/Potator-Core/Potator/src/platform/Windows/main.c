#include <stdio.h> 
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include "../sound.h"
#include "../memorymap.h"
#include "../supervision.h"
#include "../controls.h"
#include "../types.h"

#include <SDL/SDL.h>

BOOL paused = FALSE;

uint8* buffer;
unsigned int buffer_size = 0;

volatile unsigned char svFrm = 0;
volatile unsigned char xFrm = 0;
volatile unsigned char FPS = 0;

unsigned short screenbuffer[161*161];
SDL_Surface *PrimarySurface = NULL;

char *romname;

int loadROM(char *filename)
{
	FILE*	file;
	unsigned long	filesize;
	
	char ext[5];
	char *pext;
	char *p;
	
	char buf[256];
		
	file=fopen(filename,"rb");
	fseek(file,0,SEEK_END);
	filesize=ftell(file);
	fseek(file,0,SEEK_SET);
	buffer = (uint8 *)malloc(filesize);
	fread(buffer,1,filesize,file);
	if (!fclose(file)) printf("fclose(): Unable to close file!\n");

    //romSize = filesize;
		
    strcpy(romname,filename);

	return 1;
}

/*void loadROM(char* filename)
{
	FILE *romfile;
	u32 length;

	if (buffer != 0)
		free(buffer);

	//strcpy(romname,filename);
	//romname=filename;

	romfile = fopen(filename, "rb");
	
	if (romfile == NULL)
		printf("fopen(): Unable to open file!\n");

	fseek(romfile, 0, SEEK_END);
	buffer_size = ftell(romfile);
	fseek(romfile, 0, SEEK_SET);

	buffer = (uint8 *)malloc(buffer_size);

	fread(buffer, buffer_size, 1, romfile);

	if (!fclose(romfile))
		printf("fclose(): Unable to close file!\n");
}*/

int Draw() {
	SDL_Surface *rawImage;
	
	rawImage = SDL_CreateRGBSurfaceFrom((void*)&screenbuffer, 160, 160, 16, 512, 0x001F, 0x03E0, 0x7C00, 0);
	if(rawImage == NULL) return 1;
	
	SDL_BlitSurface(rawImage, 0, PrimarySurface, 0);
	SDL_UpdateRect(PrimarySurface, 0, 0, 0, 0);
  
	SDL_FreeSurface(rawImage);
	return 1;
}

void InfoNES_LoadFrame()
{
/*
 *  Transfer the contents of work frame on the screen
 *
 */
	Uint16 *pDest, *pSrc;
	int x, y;

	SDL_LockSurface(PrimarySurface);
	pDest = (Uint16 *)PrimarySurface->pixels;
	pSrc = screenbuffer;

	/* Blit screen */
	for ( y = 0; y < 160; y++ )
	{
		for ( x = 0; x < 160; x++ )
		{  
			*pDest = (((*pSrc) & 0x7FE0) << 1) | ((*pSrc) & 0x001F);
			pDest++;pSrc++;
		}
		pDest += 160;
	}
	SDL_UnlockSurface(PrimarySurface);
	SDL_Flip(PrimarySurface);
}

int main(int argc, char *argv[])
{

	int i,j;
	char temp[255];
	Uint16 *ScreenMem;
	Uint32 initflags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER;  /* See documentation for details */

	
	/* Initialize the SDL library */
	if ( SDL_Init(initflags) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
				SDL_GetError());
		exit(1);
	}

	PrimarySurface = SDL_SetVideoMode(160, 160, 16, SDL_SWSURFACE);

	SDL_ShowCursor(0);
	
	SDL_FillRect(PrimarySurface, NULL, 0);
	
	SDL_WM_SetCaption("Potator SDL", NULL);
	if(!PrimarySurface) {
		puts("error");
		return 1;
	}
	
	FILE *in = NULL;
    
    if(argc <= 1) {
		printf("\nnot enough arguments\n");
		//return(TRUE);
	} else {
	
		// the hard-core UI, a command line:
		for (i=0; (i < argc || argv[i] != NULL); i++) {
		
			if(strcmp(argv[i], "--double") == 0) {
				//screen_size = 1;
			}
			if(strcmp(argv[i], "--color white") == 0) {
				supervision_set_colour_scheme(COLOUR_SCHEME_DEFAULT);
			}
			if(strcmp(argv[i], "--color amber") == 0) {
				supervision_set_colour_scheme(COLOUR_SCHEME_AMBER);
			}
			if(strcmp(argv[i], "--color green") == 0) {
				supervision_set_colour_scheme(COLOUR_SCHEME_GREEN);
			}
			if(strcmp(argv[i], "--color blue") == 0) {
				supervision_set_colour_scheme(COLOUR_SCHEME_BLUE);
			}
		}
	
		romname = strdup(argv[1]);
		in = fopen(romname, "rb");
	
		if(in == NULL) {
			printf("The file %s doesn't exist.\n",romname);
			exit(0);
		}
		fflush(in);
		fclose(in);
	}
		
	supervision_init(); //Init the emulator

	loadROM("rom.sv");
	supervision_load((u8*)buffer, (uint32)buffer_size);

	while(1)
	{

	  while(!paused)
	  {

		 controls_update();

         supervision_exec((int16*)screenbuffer,1);
		 //Draw();
		 InfoNES_LoadFrame();

		 controls_reset();
	  }
	}
	supervision_done(); //shutsdown the system
	SDL_Quit();
}
