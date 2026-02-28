#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sound.h"
#include "memorymap.h"
#include "supervision.h"
#include "controls.h"

#include <nds.h>
#include <fat.h>
 
void loadROM();

char *romname;
char *fname;

uint8* buffer;
unsigned int buffer_size = 0;

unsigned int main_version = 0;
unsigned int sub_version = 2;

uint16 screenbuffer[161*161];

void loadROM(char* filename)
{

	if (buffer != 0)
		free(buffer);

	char romfilename[500] = "/sv/";
	strcat(romfilename, filename);

	iprintf("Opening file: %s...\n", romfilename);

	FILE *romfile = fopen(romfilename, "r");

	iprintf("fopen...\n");
	
	if (romfile == (FILE *)-1)
		iprintf("fopen(): Unable to open file!\n");

	iprintf("fopen():...\n");

	fseek(romfile, 0, SEEK_END);
	iprintf("fseek()...\n");
	buffer_size = ftell(romfile);
	iprintf("ftell()...\n");
	fseek(romfile, 0, SEEK_SET);
	iprintf("fseek()...\n");

	iprintf("buffer_size...\n");

	buffer = (uint8*)malloc(buffer_size);

	fread(buffer, buffer_size, 1, romfile);

	iprintf("fread...\n");

	if (!fclose(romfile))
		iprintf("fclose(): Unable to close file!\n");
}

void ClearScreen() // Abstract this.
{
	iprintf("\x1b[2J");
}

void InitVideo(void)
{	
	powerON(POWER_ALL);

	#ifndef No_Splash
	irqInit();
	irqSet(IRQ_VBLANK, 0);
	#endif
		
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE); 
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE); 
	
	vramSetMainBanks(VRAM_A_MAIN_BG_0x6000000, VRAM_B_LCD, 
                     VRAM_C_SUB_BG , VRAM_D_LCD);

	SUB_BG0_CR = BG_MAP_BASE(31);
	BG_PALETTE_SUB[255] = RGB15(31,31,31);
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
	
	BG3_CR = BG_BMP16_256x256 | (1<<13);
	BG3_XDX = 1 << 8; BG3_XDY = 0 << 8;
	BG3_YDX = 0 << 8; BG3_YDY = 1 << 8;
	BG3_CX = -48<<8;  BG3_CY = -16<<8;
	
}

void CheckKeys(void)
{
	scanKeys();

	u32 keys = keysHeld();

	if(keys & KEY_L && keys & KEY_R) { //Checks if L and R are pushed
	supervision_reset(); //Reset emulator
	ClearScreen();
	iprintf("Watari v%d.%d\n",main_version,sub_version);
	iprintf("Ported By Normmatt\n");
	iprintf("normmatt.com\n"); }

	if(keys & KEY_L && keys & KEY_LEFT) //Checks if L and LEFT pushed
	supervision_set_colour_scheme(0); //Changes the color scheme

	if(keys & KEY_L && keys & KEY_RIGHT) //Checks if L and RIGHT are pushed
	supervision_set_colour_scheme(1); //Changes the color scheme

	if(keys & KEY_L && keys & KEY_UP) //Checks if L and UP pushed
	supervision_set_colour_scheme(2); //Changes the color scheme

	if(keys & KEY_L && keys & KEY_DOWN) //Checks if L and DOWN pushed
	supervision_set_colour_scheme(3); //Changes the color scheme

	/*if(keys & KEY_START && keys & KEY_SELECT) {
	dotextmenu();
	loadROM();
	supervision_load((u8*)buffer, (uint32)buffer_size);
	iprintf("\nLoad Rom Seccessfully\n"); }*/
}

int main()
{
	InitVideo();
	lcdSwap();
		
	int j=0;
		
	// Map Game Cartridge memory to ARM9
	WAIT_CR &= ~0x80;

	//iprintf("\x1b[17;0HPress A to Start");
	//iprintf("\x1b[2;0HWatari v1 by  Normmatt");
	//iprintf("\x1b[3;0HBased on Potator by Cal2\n\n");
	//iprintf("\x1b[20;0HROM Loader by Extreme Coder");
		
	supervision_init(); //Init the emulator

	if (fatInitDefault()) {
    loadROM("test.sv");
	} else {
	iprintf("\nFailed to init fat");
	} 

	supervision_load((u8*)buffer, (uint32)buffer_size);
	iprintf("\nLoad Rom Seccessfully\n");
		
	while(1)
	{
		CheckKeys(); //key control

		controls_update();

		supervision_exec((int16*)screenbuffer,1); //Execute the emulator
//		supervision_exec_fast((int16*)screenbuffer,1); //Execute the emulator

		for(j=0; j < 161; j++)
			dmaCopyWordsAsynch(3, screenbuffer+(j * 160), BG_GFX+(j*256), 160*2); //copy frame buffer to screen

		//Clearing Keys
		controls_reset();

	}
	supervision_done(); //shutsdown the system
}

