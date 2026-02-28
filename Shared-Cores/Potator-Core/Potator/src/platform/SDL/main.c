#include <stdio.h> 
#include <stdlib.h>
#include <stdarg.h>

#include "../../common/sound.h"
#include "../../common/memorymap.h"
#include "../../common/supervision.h"
#include "../../common/controls.h"
#include "../../common/types.h"

#include <SDL/SDL.h>

BOOL paused = FALSE;

uint8* buffer;
unsigned int buffer_size = 0;

unsigned short screenbuffer[161*161];
SDL_Surface *PrimarySurface;

char *romname;

void loadROM(char* filename)
{
	u32 length;

	if (buffer != 0)
		free(buffer);

	FILE *romfile = fopen(filename, "rb");
	
	if (romfile == (FILE *)-1)
		printf("fopen(): Unable to open file!\n");

	fseek(romfile, 0, SEEK_END);
	buffer_size = ftell(romfile);
	fseek(romfile, 0, SEEK_SET);

	buffer = (unsigned char *)malloc(buffer_size);

	uint32 bytesread = fread(buffer, buffer_size, 1, romfile);

	if (!fclose(romfile))
		printf("fclose(): Unable to close file!\n");
}

void Draw()
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
	}
	SDL_UnlockSurface(PrimarySurface);
	SDL_Flip(PrimarySurface);
}

int main(int argc, char *argv[])
{
	int i;
	/* Initialize the SDL library */
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
				SDL_GetError());
		exit(1);
	}

	PrimarySurface = SDL_SetVideoMode(160, 160, 16, SDL_SWSURFACE);
	if (PrimarySurface == NULL) {
		fprintf (stderr, "Couldn't set 160x160x16 video mode: %s\n", SDL_GetError ());
		exit (2);
	}
	
	printf("SDL_SetVideoMode();\n");

	SDL_ShowCursor(SDL_DISABLE);
	
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
	if(romname) loadROM(romname); else loadROM("rom.sv");		
	supervision_load((u8*)buffer, (uint32)buffer_size);

	while(1)
	{

	  while(!paused)
	  {
		 controls_update();

         supervision_exec((int16*)screenbuffer,1);
		 Draw();

		 controls_reset();
	  }
	}
	supervision_done(); //shutsdown the system
	SDL_Quit();
}
