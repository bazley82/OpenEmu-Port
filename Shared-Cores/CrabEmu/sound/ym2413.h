/*
    CrabEmu Modifications to this code:
      - Added typedefs and such to unglue the code from MAME's core.
      - Added mono update function for the Dreamcast port.
      - Updated to mamedev current as of September 4, 2016, backporting to C
        from C++.

    Copyright (C) 2011, 2012, 2016 Lawrence Sebald

    Modifications distributed under the same license as the rest of the code in
    this file (GPLv2 or newer).
*/

// license:GPL-2.0+
// copyright-holders:Jarek Burczynski,Ernesto Corvi
#ifndef __YM2413_H__
#define __YM2413_H__

/* MAME glue... */
#if __STDC_VERSION__ >= 199901L
#include <stdint.h>
typedef uint8_t UINT8;
typedef uint32_t UINT32;

typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
#else
#warning Check integer types, they may not be correct!

typedef unsigned char UINT8;
typedef unsigned int UINT32;

typedef signed char INT8;
typedef signed int INT32;
#endif
/* End MAME glue... */

struct OPLL_SLOT
{
    UINT32  ar;         /* attack rate: AR<<2           */
    UINT32  dr;         /* decay rate:  DR<<2           */
    UINT32  rr;         /* release rate:RR<<2           */
    UINT8   KSR;        /* key scale rate               */
    UINT8   ksl;        /* keyscale level               */
    UINT8   ksr;        /* key scale rate: kcode>>KSR   */
    UINT8   mul;        /* multiple: mul_tab[ML]        */

    /* Phase Generator */
    UINT32  phase;      /* frequency counter            */
    UINT32  freq;       /* frequency counter step       */
    UINT8   fb_shift;   /* feedback shift value         */
    INT32   op1_out[2]; /* slot1 output for feedback    */

    /* Envelope Generator */
    UINT8   eg_type;    /* percussive/nonpercussive mode*/
    UINT8   state;      /* phase type                   */
    UINT32  TL;         /* total level: TL << 2         */
    INT32   TLL;        /* adjusted now TL              */
    INT32   volume;     /* envelope counter             */
    UINT32  sl;         /* sustain level: sl_tab[SL]    */

    UINT8   eg_sh_dp;   /* (dump state)                 */
    UINT8   eg_sel_dp;  /* (dump state)                 */
    UINT8   eg_sh_ar;   /* (attack state)               */
    UINT8   eg_sel_ar;  /* (attack state)               */
    UINT8   eg_sh_dr;   /* (decay state)                */
    UINT8   eg_sel_dr;  /* (decay state)                */
    UINT8   eg_sh_rr;   /* (release state for non-perc.)*/
    UINT8   eg_sel_rr;  /* (release state for non-perc.)*/
    UINT8   eg_sh_rs;   /* (release state for perc.mode)*/
    UINT8   eg_sel_rs;  /* (release state for perc.mode)*/

    UINT32  key;        /* 0 = KEY OFF, >0 = KEY ON     */

    /* LFO */
    UINT32  AMmask;     /* LFO Amplitude Modulation enable mask */
    UINT8   vib;        /* LFO Phase Modulation enable flag (active high)*/

    /* waveform select */
    unsigned int wavetable;
};

struct OPLL_CH
{
    struct OPLL_SLOT SLOT[2];
    /* phase generator state */
    UINT32  block_fnum; /* block+fnum                   */
    UINT32  fc;         /* Freq. freqement base         */
    UINT32  ksl_base;   /* KeyScaleLevel Base step      */
    UINT8   kcode;      /* key code (for key scaling)   */
    UINT8   sus;        /* sus on/off (release speed in percussive mode)*/
};

enum {
    RATE_STEPS = (8),

    /* sinwave entries */
    SIN_BITS =       10,
    SIN_LEN  =       (1<<SIN_BITS),
    SIN_MASK =       (SIN_LEN-1),

    TL_RES_LEN =     (256),   /* 8 bits addressing (real chip) */

    /*  TL_TAB_LEN is calculated as:
     *   11 - sinus amplitude bits     (Y axis)
     *   2  - sinus sign bit           (Y axis)
     *   TL_RES_LEN - sinus resolution (X axis)
     */
    TL_TAB_LEN = (11*2*TL_RES_LEN),

    LFO_AM_TAB_ELEMENTS = 210

};

typedef struct ym2413_s {
	int tl_tab[TL_TAB_LEN];

	/* sin waveform table in 'decibel' scale */
	/* two waveforms on OPLL type chips */
	unsigned int sin_tab[SIN_LEN * 2];

	struct OPLL_CH P_CH[9];         /* OPLL chips have 9 channels*/
	UINT8   instvol_r[9];           /* instrument/volume (or volume/volume in percussive mode)*/

	UINT32  eg_cnt;                 /* global envelope generator counter    */
	UINT32  eg_timer;               /* global envelope generator counter works at frequency = chipclock/72 */
	UINT32  eg_timer_add;           /* step of eg_timer                     */
	UINT32  eg_timer_overflow;      /* envelope generator timer overlfows every 1 sample (on real chip) */

	UINT8   rhythm;                 /* Rhythm mode                  */

	/* LFO */
	UINT32  LFO_AM;
	INT32   LFO_PM;
	UINT32  lfo_am_cnt;
	UINT32  lfo_am_inc;
	UINT32  lfo_pm_cnt;
	UINT32  lfo_pm_inc;

	UINT32  noise_rng;              /* 23 bit noise shift register  */
	UINT32  noise_p;                /* current noise 'phase'        */
	UINT32  noise_f;                /* current noise period         */


/* instrument settings */
/*
    0-user instrument
    1-15 - fixed instruments
    16 -bass drum settings
    17,18 - other percussion instruments
*/
	UINT8 inst_tab[19][8];

	UINT32  fn_tab[1024];           /* fnumber->increment counter   */

	UINT8 address;                  /* address register             */

	signed int output[2];
} YM2413;

/* CrabEmu's public interface... */
YM2413 *ym2413_init(int clock, int rate);
void ym2413_shutdown(YM2413 *fm);
void ym2413_reset(YM2413 *fm);
void ym2413_write(YM2413 *fm, int a, int v);
void ym2413_update(YM2413 *fm, int16_t *buf, int samples);
void ym2413_update_mono(YM2413 *fm, int16_t buf, int samples);
unsigned char ym2413_read(YM2413 *fm, int a);

#endif /*__YM2413_H__*/
