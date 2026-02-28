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

#include "menues.h"
#include "minimal.h"

#include "memorymap.h"
#include "supervision.h"
#include "types.h"

FileEntry FileList[1024];
u32 fileCounter;

extern uint8* buffer;
extern unsigned int buffer_size;
extern void loadROM(char* filename);

#define IOBASE 0xC0000000
#define FPLLSETVREG (0x0910 >> 1)
#define SYS_CLK_FREQ 7372800
static const int clocklist[] = {166, 200, 220, 235, 250, 260, 266, 275, 285, 295, 300};
void CPUSetting(int speed);

char runPath[MAXPATHLEN];

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void exitMenu(void)
{
	supervision_done(); //shutsdown the system
	gp2x_deinit();
	exit(0);
}

void getRunDir(void)
{
	//char path[MAXPATHLEN];
    getcwd(runPath, MAXPATHLEN);
    printf("runDir -> %s\n", runPath);
}

void emu_ReadConfig(void)
{
	FILE *f;

	getcwd(currentConfig.lastRomDir, MAXPATHLEN);
    printf("currentDir -> %s\n", currentConfig.lastRomDir);

	chdir(runPath); //change to dir where you launched app

	// set default config
	memset(&currentConfig, 0, sizeof(currentConfig));
	//currentConfig.lastRomDir[0] = 0;
	currentConfig.videoMode = 0;
	currentConfig.show_fps = 0;
	currentConfig.enable_sound = 0;
	currentConfig.SoundRate = 11025;
	currentConfig.Frameskip = 0; // auto
	currentConfig.CPUclock = 1;
	currentConfig.volume = 255;
	gp2x_sound_volume(currentConfig.volume,currentConfig.volume);

	f = fopen("Potator2x.cfg", "rb");
	if (f) {
		fread(&currentConfig, 1, sizeof(currentConfig), f);
		fclose(f);
	}

	chdir(currentConfig.lastRomDir); // change back to last dir (where you loaded the rom from)

	if(currentConfig.videoMode==2) 
		gp2x_video_RGB_setscaling(160, 160);
	else
		gp2x_video_RGB_setscaling(320, 240);

	CPUSetting(clocklist[currentConfig.CPUclock]);
	//gp2x_sound_rate(currentConfig.SoundRate);
	gp2x_sound_volume(currentConfig.volume,currentConfig.volume);
	gp2x_sound_pause(1^currentConfig.enable_sound);
}


void emu_WriteConfig(void)
{
	FILE *f;

	getcwd(currentConfig.lastRomDir, MAXPATHLEN);
    printf("currentDir -> %s\n", currentConfig.lastRomDir);

	chdir(runPath); //change to dir where you launched app

	f = fopen("Potator2x.cfg", "wb");
	if (f) {
		fwrite(&currentConfig, 1, sizeof(currentConfig), f);
		fflush(f);
		fclose(f);
		sync();
	}

	chdir(currentConfig.lastRomDir); // change back to last dir (where you loaded the rom from)

	if(currentConfig.videoMode==2) 
		gp2x_video_RGB_setscaling(160, 160);
	else
		gp2x_video_RGB_setscaling(320, 240);

	CPUSetting(clocklist[currentConfig.CPUclock]);
	//gp2x_sound_rate(currentConfig.SoundRate);
	gp2x_sound_volume(currentConfig.volume,currentConfig.volume);
	gp2x_sound_pause(1^currentConfig.enable_sound);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void fillList(void)
{
	textClear();
	int i;
	int curFile = 0;
	DIR *dir = opendir(".");
		
	if(dir)
    {
       struct dirent *ent;
       while((ent = readdir(dir)) != NULL)
       {
           sprintf(FileList[curFile].fName,"%s",ent->d_name);
           if(ent->d_type==DT_DIR)
             FileList[curFile].fType = FT_DIR;
           else
             FileList[curFile].fType = FT_FILE;

		   FileList[curFile].isRunable = !strcasestr(FileList[curFile].fName, ".sv");
		   ++curFile;
        }
        closedir(dir);
		
		/*struct dirent **namelist;
	    int i,k,l,n;

	    n = scandir(".", &namelist, 0, alphasort);
	    if (n < 0) printf("scandir");

		for(i=0;i<n-1;i++)
		{
		   l=strlen(namelist[i]->d_name);
		   if(namelist[i]->d_type==DT_DIR)
             FileList[curFile].fType = FT_DIR;
           else
             FileList[curFile].fType = FT_FILE;
		   strncat (FileList[curFile].fName, namelist[i]->d_name, strlen(namelist[i]->d_name));
		   ++curFile;

		   FileList[curFile].isRunable = !strcasestr(FileList[curFile].fName, ".sv");
		}
		closedir(dir);*/
    }
    else
    {
       gp2x_printf(0,10,2,"Error opening directory\n");
       gp2x_video_RGB_flip(0);
    }
		
	fileCounter = curFile;
}

void printList(u32 startPos)
{
	u32 i = startPos;
	u32 shown;

	if(fileCounter > 18) 
		shown = 19;
	else
		shown = fileCounter;
	
	gp2x_printf(0, 1, 1, "Potator2x 1.0 by Normmatt\n\n");
		
	for(; i < startPos + shown; ++i) {
		if(FileList[i].fType == FT_DIR){
			gp2x_printf(0, 15, 15 + ((i-startPos)+1)*8, "\n<%s>", FileList[i].fName);
		} else if(FileList[i].fType == FT_FILE) {
			gp2x_printf(0, 15, 15 + ((i-startPos)+1)*8, "\n%s", FileList[i].fName);
		} else
			break;
	}
}

void handleFileMenu(void)
{

	BOOL isSelected = FALSE;
	s32 curFile = 0;
	int virtualFile = 0;
		
	fillList();
	textClear();
	while(!isSelected) {
		unsigned long pad = gp2x_joystick_read(0);
		printList(curFile);
		gp2x_printf(0, 170, 1, "VirtualFile = %d",virtualFile);
		gp2x_printf(0, 170, 10, "File Count = %d",fileCounter);

		if(FileList[virtualFile].isRunable){
			gp2x_default_font.fg = 0x6700;
			gp2x_printf(NULL, 0, (33 + (virtualFile*8)), "->");
		} else {
			gp2x_default_font.fg = 0xFFFF;
			gp2x_printf(NULL, 0, (33 + (virtualFile*8)), "->");
		}

		gp2x_default_font.fg = 0xFFFF;
			
		if(curFile)
			gp2x_printf(NULL, 200, 22, "^\n|");
			
		if((curFile + 18 != fileCounter) && fileCounter > 19)
			gp2x_printf(NULL, 200, 200, "|\nv");

		gp2x_video_RGB_flip(0);
			
		if(pad & GP2X_RIGHT) {
			virtualFile += 5;
			if(virtualFile > 18)
				curFile += virtualFile - 18;
			textClear();
		}
			
		if(pad & GP2X_LEFT) {
			virtualFile -= 5;
			if(virtualFile < 0)
				curFile += virtualFile;
			textClear();
		}
			
		if(pad & GP2X_DOWN) {
			virtualFile++;
			textClear();
		}
			
		if(pad & GP2X_UP) {
			virtualFile--;
			textClear();
		}

		if(pad & GP2X_VOL_DOWN) if(pad & GP2X_START) 
			exitMenu();
			
		if(pad & GP2X_X) {
			if(FileList[curFile + virtualFile].fType == FT_DIR) {
				chdir(FileList[curFile + virtualFile].fName);
				fillList();
				virtualFile = curFile = 0;
			} else if(FileList[curFile + virtualFile].isRunable) {
				//textClear();
				//gp2x_printf(0, 1, 1, "Loading...\n\n%s", FileList[curFile + virtualFile].fName);
				//gp2x_video_RGB_flip(0);
				RESIZE();
				loadROM(FileList[curFile + virtualFile].fName);
				textClear();
				supervision_load((u8*)buffer, (uint32)buffer_size);
				textClear();
				return;
			}
		}

		//if(pad & GP2X_A) return;
		
		if(virtualFile < 0) {
			virtualFile = 0;
			--curFile;
			if(curFile < 0)
				curFile = 0;
		}
			
		if(fileCounter < 19) {
			if(virtualFile >= fileCounter)
				virtualFile = fileCounter-1;
		} else {
			if(virtualFile > 18) {
				virtualFile = 18;
				++curFile;
				if(curFile + 18 > fileCounter)
					curFile = fileCounter - 19;
			}
		}
		
		while(pad == gp2x_joystick_read(0));
		pad = gp2x_joystick_read(0);

		textClear();
	}
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

#define OPTION_VIDEOMODE	0
#define OPTION_SHOWFPS		1
#define OPTION_ENABLESOUND	2
#define OPTION_SOUNDRATE	3
#define OPTION_FRAMESKIP	4
#define OPTION_CPUCLOCK		5
#define OPTION_VOLUME		6
#define OPTION_SAVEEXIT		7
#define OPTION_EXIT			8

char videoMode[512];
void handleMainMenu(void);

void handleOptionsMenu(void)
{

	BOOL isSelected = FALSE;
	int menuOption = 0;
		
	emu_ReadConfig();
	gp2x_video_RGB_setscaling(320, 240);
	textClear();
	while(!isSelected) {
		int clock = currentConfig.CPUclock;
		int frameskip = currentConfig.Frameskip;
		int videomode = currentConfig.videoMode;
		int vol = currentConfig.volume;
		unsigned long pad = gp2x_joystick_read(0);
		
		gp2x_printf(0, 1, 1, "Potator2x 1.0 by Normmatt\n\n");
		
		gp2x_printf(0, 15, 15 + 2*8, "videoMode      %s",videoMode);
		gp2x_printf(0, 15, 15 + 3*8, "show_fps       %d",currentConfig.show_fps);
		gp2x_printf(0, 15, 15 + 4*8, "enable_sound   %d",currentConfig.enable_sound);
		gp2x_printf(0, 15, 15 + 5*8, "SoundRate      %d",currentConfig.SoundRate);
		gp2x_printf(0, 15, 15 + 6*8, "Frameskip      %d",currentConfig.Frameskip);
		gp2x_printf(0, 15, 15 + 7*8, "CPUclock       %d",clocklist[currentConfig.CPUclock]);
		gp2x_printf(0, 15, 15 + 8*8, "volume         %d",currentConfig.volume);
		gp2x_printf(0, 15, 15 + 9*8, "Save and Exit");
		gp2x_printf(0, 15, 15 + 10*8, "Exit");

		gp2x_printf(0, 170, 1, "MenuOption = %d",menuOption);

		gp2x_printf(0, 0, (31 + (menuOption*8)), "->");

		gp2x_video_RGB_flip(0);

		if(pad & GP2X_RIGHT) {
			if(menuOption == OPTION_VIDEOMODE) if(videomode < 2) currentConfig.videoMode++;
			if(menuOption == OPTION_SHOWFPS) if(currentConfig.show_fps==0) currentConfig.show_fps=1; else currentConfig.show_fps=0;
			if(menuOption == OPTION_ENABLESOUND) if(currentConfig.enable_sound==0) currentConfig.enable_sound=1; else currentConfig.enable_sound=0;
			if(menuOption == OPTION_SOUNDRATE) currentConfig.SoundRate*=2;
			if(menuOption == OPTION_FRAMESKIP) if (frameskip < 9) frameskip++;
			if(menuOption == OPTION_CPUCLOCK) if(clock < sizeof(clocklist)) clock++;
			if(menuOption == OPTION_VOLUME) if (vol < 255) vol+= (pad & GP2X_RIGHT) ? 1 : 0;
		}
			
		if(pad & GP2X_LEFT) {
			if(menuOption == OPTION_VIDEOMODE) if(videomode > 0) currentConfig.videoMode--;
			if(menuOption == OPTION_SHOWFPS) if(currentConfig.show_fps==0) currentConfig.show_fps=1; else currentConfig.show_fps=0;
			if(menuOption == OPTION_ENABLESOUND) if(currentConfig.enable_sound==0) currentConfig.enable_sound=1; else currentConfig.enable_sound=0;
			if(menuOption == OPTION_SOUNDRATE) currentConfig.SoundRate/=2;
			if(menuOption == OPTION_FRAMESKIP) if (frameskip > 0) frameskip--;
			if(menuOption == OPTION_CPUCLOCK) if(clock > 0) clock--;
			if(menuOption == OPTION_VOLUME) if (vol > 0) vol-= (pad & GP2X_LEFT) ? 1 : 0;
		}
			
		if(pad & GP2X_DOWN) {
			menuOption++;
			textClear();
		}
			
		if(pad & GP2X_UP) {
			menuOption--;
			textClear();
		}

		if(pad & GP2X_VOL_DOWN) if(pad & GP2X_START)
			exitMenu();
			
		if(pad & GP2X_X) {
			switch(menuOption){
			  case OPTION_SAVEEXIT: 
				  emu_WriteConfig(); 
				  return;
			  case OPTION_EXIT: 
				  //gp2x_sound_rate(currentConfig.SoundRate); 
				  return;
			  default: break;
			}
		}

		switch(currentConfig.videoMode)
		{
			case 0: sprintf(videoMode,"Slow"); break;
			case 1: sprintf(videoMode,"Quick"); break;
			case 2: sprintf(videoMode,"Full Screen"); break;
			default: sprintf(videoMode,""); break;
		}

		if(currentConfig.SoundRate <= 11025) currentConfig.SoundRate = 11025;
		if(currentConfig.SoundRate >= 44100) currentConfig.SoundRate = 44100;

		currentConfig.Frameskip = frameskip;
		currentConfig.CPUclock  = clock;

		currentConfig.volume = vol;

		if(menuOption < 0) menuOption = 0;			
		if(menuOption > 8) menuOption = 8;
		
		while(pad == gp2x_joystick_read(0));
		pad = gp2x_joystick_read(0);

		textClear();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int saveSlot = 0;

#define MMOPTION_CONTINUE		0
#define MMOPTION_RESTART		1
#define MMOPTION_SELECTOR		2
#define MMOPTION_OPTIONS		3
#define MMOPTION_SAVESTATE		4
#define MMOPTION_LOADSTATE		5
#define MMOPTION_EXIT			6

void printMenuOptions()
{
	gp2x_printf(0, 1, 1, "Potator2x 1.0 by Normmatt\n\n");
		
	gp2x_printf(0, 15, 15 + 2*8, "Continue");
	gp2x_printf(0, 15, 15 + 3*8, "Reset");
	gp2x_printf(0, 15, 15 + 4*8, "File Selector");
	gp2x_printf(0, 15, 15 + 5*8, "Options");
	gp2x_printf(0, 15, 15 + 6*8, "Save State %d",saveSlot);
	gp2x_printf(0, 15, 15 + 7*8, "Load State %d",saveSlot);
	gp2x_printf(0, 15, 15 + 8*8, "Exit");
}

void handleMainMenu(void)
{

	BOOL isSelected = FALSE;
	int menuOption = 0;
		
	textClear();
	gp2x_video_RGB_setscaling(320, 240);
	while(!isSelected) {
		unsigned long pad = gp2x_joystick_read(0);
		printMenuOptions();
		gp2x_printf(0, 170, 1, "MenuOption = %d",menuOption);

		gp2x_printf(0, 0, (31 + (menuOption*8)), "->");

		gp2x_video_RGB_flip(0);

		if(pad & GP2X_RIGHT) {
			if((menuOption == MMOPTION_SAVESTATE) || (menuOption == MMOPTION_LOADSTATE))
				saveSlot++;
		}
			
		if(pad & GP2X_LEFT) {
			if((menuOption == MMOPTION_SAVESTATE) || (menuOption == MMOPTION_LOADSTATE))
				saveSlot--;
		}
			
		if(pad & GP2X_DOWN) {
			menuOption++;
			textClear();
		}
			
		if(pad & GP2X_UP) {
			menuOption--;
			textClear();
		}

		if(pad & GP2X_VOL_DOWN) if(pad & GP2X_START)
			exitMenu();
			
		if(pad & GP2X_X) {
			switch(menuOption){
			  case MMOPTION_CONTINUE: RESIZE(); textClear(); return;
			  case MMOPTION_RESTART: RESIZE(); supervision_reset(); textClear(); return;
			  case MMOPTION_SELECTOR: handleFileMenu(); return;
			  case MMOPTION_OPTIONS: handleOptionsMenu(); textClear(); return;
			  case MMOPTION_SAVESTATE: sv_saveState(romname,saveSlot); textClear();return;
			  case MMOPTION_LOADSTATE: sv_loadState(romname,saveSlot); textClear();return;
			  case MMOPTION_EXIT: exitMenu(); break;
			  default: return;
			}
		}

		/*if(pad & GP2X_B) {
			textClear(); 
			return;
		}*/
		
		if(menuOption < 0) menuOption = 0;			
		if(menuOption > 6) menuOption = 6;

		if(saveSlot < 0) saveSlot = 0;
		if(saveSlot > 9) saveSlot = 9;
		
		while(pad == gp2x_joystick_read(0));
		pad = gp2x_joystick_read(0);

		textClear();
	}
}

void CPUSetting(int speed)
{
	volatile unsigned short *value;
	unsigned short mdiv = 0;
	unsigned short pdiv = 3;
	unsigned short scale = 0;
	unsigned short wrt;
	static int last_speed = -1;
	int adjusted;
	int mem_fd;

	if(speed == last_speed)
		return;

	mdiv = (pdiv * speed*1000*1000) / SYS_CLK_FREQ;
	adjusted = (mdiv * SYS_CLK_FREQ) / (pdiv * 1000*1000);
	mdiv = ((mdiv-8)<<8) & 0xff00;
	pdiv = ((pdiv-2)<<2) & 0xfc;
	scale &= 3;
	wrt = mdiv | pdiv | scale;

	mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
	if(mem_fd > 0) {
		value = (unsigned short *) mmap( 0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, IOBASE );
		if(value) {
			value[FPLLSETVREG] = wrt;
			printf ("Clock Adjust : %d MHz(%d MHz)\n", speed, adjusted);
			//INI_WriteInt("system", "clock", speed);
			last_speed = speed;
			munmap((void*)value, 0x10000);
		}
		close(mem_fd);
	}
}
