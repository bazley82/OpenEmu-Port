// VecXGL 1.2 (SDL/Win32 and SDL/Linux)
//
// This is a port of the vectrex emulator "vecx", by Valavan Manohararajah.
// Portions of this code copyright James Higgs 2005/2007.
// These portions are:
// 1. Ay38910 PSG (audio) emulation wave-buffering code.
// 2. Drawing of vectors using OpenGL.
//
// Comand-line parsing code gratefully borrowed from vecxsdl (Thomas Mathys).
// Key mapping and command-line options were also changed
// to be compatible with Thomas Mathys' vecxsdl.
//
// Other vecx ports by JH:
// - VecXPS2 (Playsyation 2)
// - VecXWin32 (Windows/DirectX) (unreleased)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "vecx.h"
#include "bios.h"						// bios rom data
#include "wnoise.h"						// White noise waveform
#include "overlay.h"					// overlay texture info
#import "osint.h"
#import "VectrexGameCore.h"
#import "e8910.h"

//typedefs for integers
typedef uint8_t Uint8;
typedef uint16_t Uint16;

// AY38910 emulation stuff
extern unsigned snd_regs[16];
Uint8 AY_vol[3];
Uint16 AY_spufreq[3];
Uint16 AY_noisefreq;
Uint8 AY_tone_enable[3];
Uint8 AY_noise_enable[3];

//Audio buffer
Uint8 *pWave;

static const char* cartname = NULL;

static long screen_x = DEFAULT_WIDTH;
static long screen_y = DEFAULT_HEIGHT;
static long scl_factor;

//static long bytes_per_pixel;
static GLfloat VX_color_set[VECTREX_COLORS];
static GLfloat line_width = DEFAULT_LINEWIDTH;
static GLfloat overlay_transparency = DEFAULT_OVERLAYTRANSPARENCY;

// Global texture image info
TextureImage g_overlay;							// Storage For One Texture
extern int LoadTGA (char *filename);			// Loads A TGA File Into Memory

static void osint_updatescale (void)
{
	long sclx, scly;

	sclx = ALG_MAX_X / screen_x;
	scly = ALG_MAX_Y / screen_y;

	if (sclx > scly) {
		scl_factor = sclx;
	} else {
		scl_factor = scly;
	}
}

void openCart(const char *romName)
{
	FILE *cartfile;
	cartname = romName;
	cartfile = fopen (cartname, "rb");
    
	if (cartfile != NULL) {
        fread (cart, 1, sizeof (cart), cartfile);
        fclose (cartfile);
	}	
}


int osint_defaults (void)
{
	unsigned b;

	screen_x = DEFAULT_WIDTH;
	screen_y = DEFAULT_HEIGHT;

	osint_updatescale ();

	// JH - built-in bios
	memcpy(rom, bios_data, bios_data_size);

	/* the cart is empty by default */
	for (b = 0; b < sizeof (cart); b++) {
		cart[b] = 0;
	}
    
    e8910_init_sound();
    
    //initialize and zero audio buffer
    pWave = malloc(882);
    memset(pWave, 0, 882);
    
    g_overlay.width = 0;

	return 0;
}

// Load a custom vectrex bios rom
static void osint_load_bios(const char *filename) {

	FILE *f;

	f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "Can't open bios image (%s).\n", filename);
		exit(1);
	}

	if (sizeof(rom) != fread(rom, 1, sizeof(rom), f)) {
		fprintf(
			stderr,
			"%s is not a valid Vectrex BIOS.\n"
			"It's smaller than %lu bytes.\n",
			filename, sizeof(rom)
		);
		fclose(f);
		exit(1);
	}

	fclose(f);
}

static void osint_maskinfo (int mask, int *shift, int *precision)
{
	*shift = 0;

	while ((mask & 1L) == 0) {
		mask >>= 1;
		(*shift)++;
	}

	*precision = 0;

	while ((mask & 1L) != 0) {
		mask >>= 1;
		(*precision)++;
	}
}

void osint_gencolors (void)
{
	int c;
	int rcomp, gcomp, bcomp;

	for (c = 0; c < VECTREX_COLORS; c++) {
		rcomp = c * 256 / VECTREX_COLORS;
		gcomp = c * 256 / VECTREX_COLORS;
		bcomp = c * 256 / VECTREX_COLORS;

		VX_color_set[c] = (GLfloat)c/128;
		if(VX_color_set[c] > 1.0f) VX_color_set[c] = 1.0f;
	}
}

/*
    JH - there some were nice low-level line drawing routines here,
         which have been replaced by OpenGL calls
*/

void osint_render (void)
{
	// GL rendering code by James Higgs
	int     width, height;
	long v;
	GLfloat c;
	//GLfloat alpha;

    // Get window size (may be different than the requested size)
	width = (int)screen_x;
	height = (int)screen_y;

    height = height > 0 ? height : 1;

    // Set viewport
    glViewport( 0, 0, width, height );
	glScissor( 0, 0, width, height );

	// draw overlay or clear screen if no overlay is used
    if (g_overlay.width > 0)
    {
        // create texture
        glShadeModel(GL_SMOOTH);
        glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
        glClearDepth(1.0f);
        glBindTexture(GL_TEXTURE_2D, g_overlay.texID);
        //

		GLfloat alpha = overlay_transparency;
		glColor3f(alpha, alpha, alpha);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
			if (g_overlay.upsideDown)
			{
				glTexCoord2f(1, 1); //0.8f, 1);
				glVertex2f(ALG_MAX_X, 0);
				glTexCoord2f(0, 1); //0.2f, 1);
				glVertex2f(0, 0);
				glTexCoord2f(0, 0); //0.2f, 0);
				glVertex2f(0, ALG_MAX_Y);
				glTexCoord2f(1, 0); //0.8f, 0);
				glVertex2f(ALG_MAX_X, ALG_MAX_Y);
			}
			else
			{
				glTexCoord2f(1, 0); //0.8f, 1);
				glVertex2f(ALG_MAX_X, 0);
				glTexCoord2f(0, 0); //0.2f, 1);
				glVertex2f(0, 0);
				glTexCoord2f(0, 1); //0.2f, 0);
				glVertex2f(0, ALG_MAX_Y);
				glTexCoord2f(1, 1); //0.8f, 0);
				glVertex2f(ALG_MAX_X, ALG_MAX_Y);
			}
		glEnd();
		glDisable(GL_TEXTURE_2D);
	} else {
	    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		glClear(GL_COLOR_BUFFER_BIT);
	}

    // Select and setup the projection matrix
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	glOrtho( 0, -33000, 41000, 0, 1.0, 50.0 );

    // Select and setup the modelview matrix
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    gluLookAt( 0.0f, 0.0f, -10.0f,    // Eye-position
               0.0f, 0.0f, 0.0f,   // View-point
               0.0f, 1.0f, 0.0f );  // Up-vector

	glEnable(GL_LINE_SMOOTH);
	glLineWidth(line_width);
	glEnable(GL_POINT_SMOOTH);
	glPointSize(line_width);

	// blend lines with overlay image
	if (g_overlay.width > 0) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR, GL_ONE);
	}

    glBegin( GL_LINES );

	// draw lines for this frame
	for (v = 0; v < vector_draw_cnt; v++) {
		c = VX_color_set[vectors_draw[v].color];
        
		glColor4f( c, c, c, 0.75f );
		glVertex3i( (int)vectors_draw[v].x0, (int)vectors_draw[v].y0, 0 );
		glVertex3i( (int)vectors_draw[v].x1, (int)vectors_draw[v].y1, 0 );

	}

	glEnd();

	// we have to redraw points, because zero-length line doesn't get drawn
	glBegin(GL_POINTS);
	for (v = 0; v < vector_draw_cnt; v++) {
		c = VX_color_set[vectors_draw[v].color];
		glColor3f( c,c,c );
		glVertex3i( (int)vectors_draw[v].x0, (int)vectors_draw[v].y0, 0 );
		glVertex3i( (int)vectors_draw[v].x1, (int)vectors_draw[v].y1, 0 );
	}

	glEnd();

	glDisable(GL_BLEND);

    }

void osint_btnDown(OEVectrexButton btn) {
    switch(btn) {
        case OEVectrexButton1:
            snd_regs[14] &= ~0x01;
            break;
        case OEVectrexButton2:
            snd_regs[14] &= ~0x02;
            break;
        case OEVectrexButton3:
            snd_regs[14] &= ~0x04;
            break;
        case OEVectrexButton4:
            snd_regs[14] &= ~0x08;
            break;
        case OEVectrexAnalogUp:
            alg_jch1 = 0xFF;
            break;
        case OEVectrexAnalogDown:
            alg_jch1 = 0x00;
            break;
        case OEVectrexAnalogLeft:
            alg_jch0 = 0x00;
            break;
        case OEVectrexAnalogRight:
            alg_jch0 = 0xFF;
            break;
        default:
            break;
    }
}

void osint_btnUp(OEVectrexButton btn) {
    switch(btn) {
        case OEVectrexButton1:
            snd_regs[14] |= 0x01;
            break;
        case OEVectrexButton2:
            snd_regs[14] |= 0x02;
            break;
        case OEVectrexButton3:
            snd_regs[14] |= 0x04;
            break;
        case OEVectrexButton4:
            snd_regs[14] |= 0x08;
            break;
        case OEVectrexAnalogUp:
            alg_jch1 = 0x80;
            break;
        case OEVectrexAnalogDown:
            alg_jch1 = 0x80;
            break;
        case OEVectrexAnalogLeft:
            alg_jch0 = 0x80;
            break;
        case OEVectrexAnalogRight:
            alg_jch0 = 0x80;
            break;
        default:
            break;
    }
}

// load overlay and set it as current texture
void load_overlay(char *filename)
{
    if (!LoadTGA(filename))				// Load The Font Texture
    {
        return;										// If Loading Failed, Return False
    }
    /*
    //BuildFont();											// Build The Font

    glShadeModel(GL_SMOOTH);								// Enable Smooth Shading
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);					// Black Background
    glClearDepth(1.0f);										// Depth Buffer Setup
    glBindTexture(GL_TEXTURE_2D, g_overlay.texID);		// Select Our Font Texture
    
    //glScissor(1	,64,637,288);								// Define Scissor Region
    
    //return TRUE;
    */
}
