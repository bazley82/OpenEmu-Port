#ifndef __OSINT_H
#define __OSINT_H
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>
#include "OEVectrexSystemResponderClient.h"

#define EMU_TIMER                   20			// milliseconds per frame
#define DEFAULT_WIDTH               330 * 2
#define DEFAULT_HEIGHT              410 * 2
#define DEFAULT_LINEWIDTH           1.0f
#define DEFAULT_OVERLAYTRANSPARENCY	0.5f
#define VECTREX_AUDIO_FREQ          22050
#define VECTREX_AUDIO_CHANNELS      1
#define VECTREX_AUDIO_SAMPLES       441

void osint_render (void);
int osint_msgs (void);
void openCart(const char *romName);
int osint_defaults (void);
void osint_gencolors (void);
void osint_btnDown(OEVectrexButton btn);
void osint_btnUp(OEVectrexButton btn);
void load_overlay(char *filename);

extern uint8_t *pWave;

#endif

