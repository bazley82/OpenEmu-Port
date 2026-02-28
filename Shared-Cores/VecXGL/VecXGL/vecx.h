#ifndef __VECX_H
#define __VECX_H

enum {
	VECTREX_MHZ		= 1500000, /* speed of the vectrex being emulated */
	VECTREX_COLORS  = 128,     /* number of possible colors ... grayscale */

	ALG_MAX_X		= 33000,
	ALG_MAX_Y		= 41000
};

enum {
	VECTREX_PDECAY	= 30,      /* phosphor decay rate */
	
	/* number of 6809 cycles before a frame redraw */
    
	FCYCLES_INIT    = VECTREX_MHZ / VECTREX_PDECAY,
    
	/* max number of possible vectors that maybe on the screen at one time.
	 * one only needs VECTREX_MHZ / VECTREX_PDECAY but we need to also store
	 * deleted vectors in a single table
	 */
	
	VECTOR_CNT		= VECTREX_MHZ / VECTREX_PDECAY,
    
	VECTOR_HASH     = 65521
};

typedef struct vector_type {
	long x0, y0; /* start coordinate */
	long x1, y1; /* end coordinate */

	/* color [0, VECTREX_COLORS - 1], if color = VECTREX_COLORS, then this is
	 * an invalid entry and must be ignored.
	 */
	unsigned char color;
} vector_t;

typedef struct VECXState {
	//e6809 cpu regs
	unsigned cpuRegs[10];
	//vectrex ram
	unsigned char ram[1024];
	//sound chip
	unsigned sndRegs[16];
	unsigned sndSelect;
	//via 6522 regs
	unsigned viaRegs[25];
	//analog stuff
	unsigned analogDevices[10];
	long analogAlg[4];
	//vectoring stuff
	unsigned algVectoring;
	long vectorPoints[6];
	unsigned char vecColor;
	long vecDrawInfo[2];
} VECXState;

extern unsigned char ram[1024];
extern unsigned char rom[8192];
extern unsigned char cart[32768];

extern unsigned snd_regs[16];
extern unsigned alg_jch0;
extern unsigned alg_jch1;
extern unsigned alg_jch2;
extern unsigned alg_jch3;

extern long vector_draw_cnt;
extern long vector_erse_cnt;
extern vector_t *vectors_draw;
extern vector_t *vectors_erse;

void vecx_reset (void);
void vecx_emu (long cycles, int ahead);

VECXState * saveVecxState();
void loadVecxState(VECXState *state);

#endif


