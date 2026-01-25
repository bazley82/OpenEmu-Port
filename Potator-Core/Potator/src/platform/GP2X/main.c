#include <dirent.h> 
#include <sys/types.h> 
#include <sys/param.h> 
#include <sys/stat.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <stdarg.h>

#include "sound.h"
#include "memorymap.h"
#include "supervision.h"
#include "controls.h"
#include "menues.h"
#include "types.h"

#include "minimal.h"

BOOL paused = FALSE;

uint8* buffer;
unsigned int buffer_size = 0;

volatile unsigned char svFrm = 0;
volatile unsigned char xFrm = 0;
volatile unsigned char FPS = 0;

unsigned short *screen16;
unsigned short screenbuffer[161*161];

char *romname;

currentConfig_t currentConfig;

void loadROM(char* filename)
{
	u32 length;

	if (buffer != 0)
		free(buffer);

	//strcpy(romname,filename);
	romname=filename;

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

void CheckKeys(void)
{
	unsigned long  pad=gp2x_joystick_read(0);

	if(pad & GP2X_VOL_DOWN) if(pad & GP2X_START) {
		supervision_done(); //shutsdown the system
		//gp2x_deinit();
		exit(0);
	}

	if(pad & GP2X_L) if(pad & GP2X_R) //Checks if L and R are pushed
	supervision_reset(); //Reset emulator

	if(pad & GP2X_L) if(pad & GP2X_LEFT) //Checks if L and LEFT pushed
	supervision_set_colour_scheme(COLOUR_SCHEME_DEFAULT); //Changes the color scheme

	if(pad & GP2X_L) if(pad & GP2X_RIGHT) //Checks if L and RIGHT are pushed
	supervision_set_colour_scheme(COLOUR_SCHEME_AMBER); //Changes the color scheme

	if(pad & GP2X_L) if(pad & GP2X_UP) //Checks if L and UP pushed
	supervision_set_colour_scheme(COLOUR_SCHEME_GREEN); //Changes the color scheme

	if(pad & GP2X_L) if(pad & GP2X_DOWN) //Checks if L and DOWN pushed
	supervision_set_colour_scheme(COLOUR_SCHEME_BLUE); //Changes the color scheme

	if(pad & GP2X_Y) {
	paused=TRUE;
	textClear();
	handleMainMenu(); // File menu
	paused=FALSE;}

	if (pad & (GP2X_VOL_UP|GP2X_VOL_DOWN)) {
		int vol = currentConfig.volume;
		if (pad & GP2X_VOL_UP) {
			if (vol < 255) vol++;
		} else {
			if (vol >   0) vol--;
		}
		gp2x_sound_volume(vol, vol);
		currentConfig.volume = vol;
	}
}

int main(int argc, char *argv[])
{
	gp2x_init(1000, 16, 11025,16,1,60, 1);
	gp2x_sound_volume(100,100);

	screen16 = (unsigned short *)gp2x_video_RGB[0].screen;

	int i,j;
	char temp[255];

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
		in = fopen(romname, "r");
	
		if(in == NULL) {
			printf("The file %s doesn't exist.\n",romname);
			exit(0);
		}
		fflush(in);
		fclose(in);
	}
		
	supervision_init(); //Init the emulator

	getRunDir();

	if(romname!=NULL){
		loadROM(romname);
		supervision_load((u8*)buffer, (uint32)buffer_size);
	} else {
		handleFileMenu(); // File menu
	}

	emu_ReadConfig();

	gp2x_sound_volume(255,255);
	gp2x_sound_pause(0);

	while(1)
	{
	  CheckKeys();

	  while(!paused)
	  {
		 CheckKeys(); //key control

		 controls_update();

		 switch(currentConfig.videoMode){
			case 0: 
				supervision_exec((int16*)screenbuffer,1);

				for(j=0; j < 160; j++) 
		 			gp2x_memcpy(screen16+(80+(j+40)*320),screenbuffer+(j * 160),160*2);
				break;
			case 1: 
				supervision_exec2((int16*)screen16,1); 
				break;
			case 2: 
				supervision_exec3((int16*)screen16,1);
				break;
			default: break;
		 }

		 /*gp2x_video_waitvsync();

		 sprintf(temp,"FPS: %3d", FPS);
		 gp2x_printf(NULL,0,0,temp);
		 ++svFrm;*/

		 gp2x_video_RGB_flip(0);


		 controls_reset();
	  }
	}
	supervision_done(); //shutsdown the system
	gp2x_deinit();
}


extern uint8 noise_buffer[11025];
extern uint8 voice_buffer[11025];
extern uint8 dma_buffer[11025];

extern BOOL audio_FmVoicePlaying;
extern BOOL audio_NoiseVoicePlaying;
extern BOOL audio_AudioDmaVoicePlaying;

void gp2x_sound_frame(void *blah, void *buffer, int samples)
{
 int i = 0;
 signed short *buffer16=buffer;

 while(samples--)
 {

	 if(currentConfig.enable_sound){
		//if(audio_FmVoicePlaying && audio_NoiseVoicePlaying && audio_AudioDmaVoicePlaying) {
			*buffer16++=(signed short)(noise_buffer[i]+voice_buffer[i]+dma_buffer[i]);	//Left
			*buffer16++=(signed short)(noise_buffer[i]+voice_buffer[i]+dma_buffer[i]);	//Right
		/*} else if(audio_FmVoicePlaying && audio_NoiseVoicePlaying) {
			*buffer16++=(signed short)(voice_buffer[i]+noise_buffer[i]);	//Left
			*buffer16++=(signed short)(voice_buffer[i]+noise_buffer[i]);	//Right
		} else if(audio_AudioDmaVoicePlaying && audio_NoiseVoicePlaying) {
			*buffer16++=(signed short)(dma_buffer[i]+noise_buffer[i]);	//Left
			*buffer16++=(signed short)(dma_buffer[i]+noise_buffer[i]);	//Right
		} else if(audio_AudioDmaVoicePlaying && audio_FmVoicePlaying) {
			*buffer16++=(signed short)(dma_buffer[i]+voice_buffer[i]);	//Left
			*buffer16++=(signed short)(dma_buffer[i]+voice_buffer[i]);	//Right
		} else if(audio_FmVoicePlaying) {
			*buffer16++=(signed short)(voice_buffer[i]);	//Left
			*buffer16++=(signed short)(voice_buffer[i]);	//Right
		} else if(audio_AudioDmaVoicePlaying) {
			*buffer16++=(signed short)(dma_buffer[i]);	//Left
			*buffer16++=(signed short)(dma_buffer[i]);	//Right
		} else if(audio_NoiseVoicePlaying) {
			*buffer16++=(signed short)(noise_buffer[i]);	//Left
			*buffer16++=(signed short)(noise_buffer[i]);	//Right
		}*/
		++i;
	 } else {
		*buffer16++=0;	//Left
		*buffer16++=0;	//Right
	 }
 }

}
