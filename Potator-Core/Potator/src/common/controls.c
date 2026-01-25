
#include <stdlib.h>
#include <stdio.h>
#include "controls.h"

#ifdef GP2X
#include "minimal.h"
#endif
#ifdef NDS
#include <nds.h>
#endif
#ifdef _SDL_
#include <SDL/SDL.h>
#endif

uint8 controls_state;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void controls_init()
{
	//fprintf(log_get(), "controls: init\n");
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
void controls_done()
{
	//fprintf(log_get(), "controls: done\n");
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
void controls_reset()
{
	//fprintf(log_get(), "controls: reset\n");
	controls_state = 0x00;
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
void controls_write(uint32 addr, uint8 data)
{
}

void controls_state_write(uint8 type, uint8 data)
{
     if(controls_state==data) 
        return; 
     else 
        controls_state = 0;
     
     if(type)
       controls_state|=data;
     else
       controls_state=data;
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
uint8 controls_read(uint32 addr)
{
	return(controls_state^0xff); 
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
BOOL controls_update(void)
{
	controls_state = 0;

#ifdef GP2X	
	unsigned long  pad=gp2x_joystick_read(0);

	if (pad & GP2X_UP)			controls_state|=0x08;
	if (pad & GP2X_RIGHT)		controls_state|=0x01;
	if (pad & GP2X_LEFT)		controls_state|=0x02;
	if (pad & GP2X_DOWN)		controls_state|=0x04;
	if (pad & GP2X_UP)			controls_state|=0x08;
	if (pad & GP2X_X)			controls_state|=0x10;
	if (pad & GP2X_B)			controls_state|=0x20;
	if (pad & GP2X_START)		controls_state|=0x80;
	if (pad & GP2X_SELECT)		controls_state|=0x40;
#endif
#ifdef NDS
	if (!(REG_KEYINPUT & KEY_RIGHT))	controls_state|=0x01;
	if (!(REG_KEYINPUT & KEY_LEFT))		controls_state|=0x02;
	if (!(REG_KEYINPUT & KEY_DOWN))		controls_state|=0x04;
	if (!(REG_KEYINPUT & KEY_UP))		controls_state|=0x08;
	if (!(REG_KEYINPUT & KEY_A))		controls_state|=0x10;
	if (!(REG_KEYINPUT & KEY_B))		controls_state|=0x20;
	if (!(REG_KEYINPUT & KEY_SELECT))	controls_state|=0x40;
	if (!(REG_KEYINPUT & KEY_START))	controls_state|=0x80;
#endif
/*#ifdef _SDL_ 
	uint8 *keystate = SDL_GetKeyState(NULL);

	if (keystate[SDLK_RIGHT])	controls_state|=0x01;
	if (keystate[SDLK_LEFT])	controls_state|=0x02;
	if (keystate[SDLK_DOWN])	controls_state|=0x04;
	if (keystate[SDLK_UP])		controls_state|=0x08;
	if (keystate[SDLK_x])		controls_state|=0x10;
	if (keystate[SDLK_c])		controls_state|=0x20;
	if (keystate[SDLK_F1])		controls_state|=0x80;
	if (keystate[SDLK_F2])		controls_state|=0x40;
#endif*/
#ifdef _SDL_    
    SDL_Event event;

	// Check for events
	while( SDL_PollEvent( &event ) )
	{
	    switch( event.type )
		{
			case SDL_KEYDOWN:
				switch( event.key.keysym.sym )
				{
					case SDLK_RIGHT:
                        controls_state|=0x01;
						break;
					case SDLK_LEFT:
                        controls_state|=0x02;
						break;
					case SDLK_DOWN:
                        controls_state|=0x04;
						break;
					case SDLK_UP:
                        controls_state|=0x08;
						break;
					case SDLK_x:
                        controls_state|=0x10;
						break;
					case SDLK_c:
                        controls_state|=0x20;
						break;
					case SDLK_F1:
                        controls_state|=0x40;
						break;
					case SDLK_F2:
                        controls_state|=0x80;
						break;
					default:
						break;
				}
			break;
		}
	}
#endif
	return(TRUE);
}
