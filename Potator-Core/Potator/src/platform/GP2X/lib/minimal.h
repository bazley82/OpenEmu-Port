/* GP2X minimal library v0.C
   Written by rlyeh, (c) 2005-2006.

   Please check readme for licensing and other conditions. */

#ifndef __MINIMAL_H__
#define __MINIMAL_H__

#include <math.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <linux/keyboard.h>
#include <linux/kd.h>
#include <linux/joystick.h>


#define MINILIB_VERSION  "GP2X minimal library (internal beta v0.C) by rlyeh, (c) 2005-2006."


#ifndef GP2X_DEBUG_LEVEL
#define GP2X_DEBUG_LEVEL 0
#endif

#ifdef  GP2X_PROFILE_DISABLE
#define GP2X_PROFILE 0
#else
#define GP2X_PROFILE 1
#endif

#define gp2x_debug(level, func)   if(GP2X_DEBUG_LEVEL & level) { func } 

#define gp2x_profile(func) \
 if(GP2X_PROFILE) {                                                                          \
 static gp2x_profiles *prf = NULL;                                                           \
                                                                                             \
 if(prf == NULL) { prf = gp2x_profile_register();                                            \
                   sprintf(prf->fname,  __FILE__ ":%d:%s", __LINE__, __func__ );             \
                   prf->time = prf->calls = 0; }                                          \
                                                                                             \
 prf->calls++;                                                                               \
 prf->time -= gp2x_memregl[0x0A00>>2];                                                       \
 { func ;}                                                                                   \
 prf->time += gp2x_memregl[0x0A00>>2]; } else { func ;}


enum  { RECT_RGB8=1, RECT_RGB16=2, RECT_YUV=4 };

enum  { GP2X_UP=(0x1L<<0),    GP2X_LEFT=(0x1L<<2),    GP2X_DOWN=(0x1L<<4), GP2X_RIGHT=(0x1L<<6),
        GP2X_START=(1L<<8),   GP2X_SELECT=(1L<<9),    GP2X_L=(1L<<10),     GP2X_R=(1L<<11),
        GP2X_A=(1L<<12),      GP2X_B=(1L<<13),        GP2X_X=(1L<<14),     GP2X_Y=(1L<<15),
        GP2X_VOL_UP=(1L<<23), GP2X_VOL_DOWN=(1L<<22), GP2X_PUSH=(1L<<27) };

typedef struct gp2x_font        { int x,y,w,wmask,h,fg,bg,solid; unsigned char *data; } gp2x_font;
typedef struct gp2x_queue       { volatile unsigned long head, tail, items, max_items; unsigned long *place920t, *place940t; } gp2x_queue;
typedef struct gp2x_rect        { int x,y,w,h,solid,type; void *data; } gp2x_rect;

typedef struct gp2x_video_layer { void *screen;

                                  unsigned long i[8];  //physical addresses of each buffer. they might have other usages too
                                  void *p[8];          //virtual address of each buffer
                                } gp2x_video_layer;

typedef struct gp2x_file        { int size, compressed; void *data; } gp2x_file;

typedef struct gp2x_profiles    { char fname[56]; unsigned long calls, time; } gp2x_profiles;

extern gp2x_video_layer         gp2x_video_RGB[1], gp2x_video_YUV[4];
extern volatile unsigned long  *gp2x_dualcore_ram, *gp2x_memregl, *gp2x_blitter;
extern unsigned long            gp2x_usbjoys;
extern gp2x_font                gp2x_default_font;
//extern gp2x_profiles           *gp2x_profiles;


extern void           gp2x_video_setgammacorrection(float);
extern void           gp2x_video_setdithering(int);
extern void           gp2x_video_setluminance(int, int);
extern void           gp2x_video_waitvsync(void);
extern void           gp2x_video_waithsync(void);
extern void           gp2x_video_cursor_show(int);
extern void           gp2x_video_cursor_move(int, int);
extern void           gp2x_video_cursor_setalpha(int, int);
extern void           gp2x_video_cursor_setup(unsigned char *, int, unsigned char, int, int, int, int, unsigned char, int, int, int, int);
extern void           gp2x_video_logo_enable(int);

extern void           gp2x_video_RGB_color8 (int, int, int, int);
extern unsigned short gp2x_video_RGB_color15(int, int, int, int);
extern unsigned short gp2x_video_RGB_color16(int, int, int);
extern void           gp2x_video_RGB_setpalette(void);
extern void           gp2x_video_RGB_setcolorkey(int, int, int);
extern void           gp2x_video_RGB_setscaling(int, int);
extern void           gp2x_video_RGB_flip(int);
extern void           gp2x_video_RGB_setwindows(int, int, int, int, int, int);

extern unsigned long  gp2x_video_YUV_color(int, int, int);
extern void           gp2x_video_YUV_setscaling(int, int, int);
extern void           gp2x_video_YUV_flip(int);
extern void           gp2x_video_YUV_setparts(int, int, int, int, int, int);

extern void           gp2x_blitter_rect(gp2x_rect *);
extern void           gp2x_blitter_wait(void);
extern void           gp2x_blitter_send(gp2x_rect *);

extern unsigned long  gp2x_joystick_read(int);
extern char          *gp2x_joystick_name(int);
extern void           gp2x_joystick_wait(int, unsigned long);
extern void           gp2x_joystick_scan(void);

extern unsigned long  gp2x_timer_read(void);
extern void           gp2x_timer_delay(unsigned long);
extern unsigned long  gp2x_timer_raw(void);
extern unsigned long  gp2x_timer_raw_to_ticks(unsigned long);
extern unsigned long  gp2x_timer_raw_one_second(void);
extern void           gp2x_timer_init(int);
extern void           gp2x_timer_start(int);
extern void           gp2x_timer_stop(int);
extern unsigned long  gp2x_timer_cpu_usage_relative(int, int);
extern unsigned long  gp2x_timer_cpu_usage_per_frame(int);

extern gp2x_profiles *gp2x_profile_register(void);
extern char          *gp2x_profile_analyze(void);

extern void           gp2x_tv_setmode(unsigned char);
extern void           gp2x_tv_adjust(signed char, signed char);

extern void           gp2x_sound_frame (void *, void *, int);
extern void           gp2x_sound_volume(int, int);
extern void           gp2x_sound_pause(int);
extern void           gp2x_sound_stereo(int);
extern void           gp2x_sound_3Dboost(int);
extern void           gp2x_sound_attenuation(int);
extern void           gp2x_sound_setintensity(int, int);

extern void           gp2x_i2c_write(unsigned char, unsigned char, unsigned char);
extern unsigned char  gp2x_i2c_read(unsigned char, unsigned char);

extern void           gp2x_dualcore_clock(int);
extern void           gp2x_dualcore_pause(int);
extern void           gp2x_dualcore_sync(void);
extern void           gp2x_dualcore_exec(unsigned long);
extern void           gp2x_dualcore_launch_program(unsigned long *, unsigned long);
extern void           gp2x_dualcore_launch_program_from_disk(char *, unsigned long, unsigned long);
extern void           gp2x_dualcore_launch_HH(char *);

extern void           gp2x_printf(gp2x_font *, int, int, const char *, ...);
extern void           gp2x_printf_init(gp2x_font *, int, int, void *, int, int, int);

extern void           gp2x_misc_led(int);
extern void           gp2x_misc_lcd(int);
extern  int           gp2x_misc_battery(void);
extern void           gp2x_misc_flushcache(void);
extern  int           gp2x_misc_headphones(void);
extern void           gp2x_misc_memcard(void);

extern  int           gp2x_image_tga(FILE *fp, gp2x_rect *r);
extern  int           gp2x_image_bmp(FILE *fp, gp2x_rect *r, unsigned char paletteIndex);
extern void           gp2x_unimage(gp2x_rect *r);

extern void           gp2x_init(int, int, int, int, int, int, int);
extern void           gp2x_deinit(void);
extern void           gp2x_reboot(void);





/* for our minimal kernel module */
#define MINIMAL_I2C_WRITE            0x0
#define MINIMAL_I2C_READ             0x1
#define MINIMAL_MMU_FLUSHCACHE       0x2
#define MINIMAL_MMU_INSTALLHACK      0x3
#define MINIMAL_MMU_INSTALLHACK_32   0x4
#define MINIMAL_MMU_INSTALLHACK_48   0x5
#define MINIMAL_MMU_UNINSTALLHACK    0x6
#define MINIMAL_MMU_R001             0x100
#define MINIMAL_MMU_R100             0x101
#define MINIMAL_MMU_W100             0x102
#define MINIMAL_MMU_R200             0x103
#define MINIMAL_MMU_W200             0x104
#define MINIMAL_MMU_R300             0x105
#define MINIMAL_MMU_W300             0x106
#define MINIMAL_MMU_R500             0x107
#define MINIMAL_MMU_W500             0x108
#define MINIMAL_MMU_R501             0x109
#define MINIMAL_MMU_W501             0x10a
#define MINIMAL_MMU_R600             0x10b
#define MINIMAL_MMU_W600             0x10c
#define MINIMAL_MMU_W750             0x10d
#define MINIMAL_MMU_W760             0x10e
#define MINIMAL_MMU_W770             0x10f
#define MINIMAL_MMU_W751             0x110
#define MINIMAL_MMU_W7131            0x111
#define MINIMAL_MMU_W761             0x112
#define MINIMAL_MMU_W7101            0x113
#define MINIMAL_MMU_W7141            0x114
#define MINIMAL_MMU_W7102            0x115
#define MINIMAL_MMU_W7104            0x116
#define MINIMAL_MMU_W7142            0x117
#define MINIMAL_MMU_W704             0x118
#define MINIMAL_MMU_W870             0x119
#define MINIMAL_MMU_W850             0x11a
#define MINIMAL_MMU_W851             0x11b
#define MINIMAL_MMU_W860             0x11c
#define MINIMAL_MMU_W861             0x11d
#define MINIMAL_MMU_R900             0x11e
#define MINIMAL_MMU_W900             0x11f
#define MINIMAL_MMU_R901             0x120
#define MINIMAL_MMU_W901             0x121
#define MINIMAL_MMU_R1000            0x122
#define MINIMAL_MMU_W1000            0x123
#define MINIMAL_MMU_R1001            0x124
#define MINIMAL_MMU_W1001            0x125
#define MINIMAL_MMU_R1300            0x126
#define MINIMAL_MMU_W1300            0x127
#define MINIMAL_MMU_DRAINWRITEBUFFER MINIMAL_MMU_W7104
#define MINIMAL_MMU_INVALIDATEICACHE MINIMAL_MMU_W750
#define MINIMAL_MMU_INVALIDATEDCACHE MINIMAL_MMU_W760





#define GP2X_QUEUE_MAX_ITEMS           ((4096 - sizeof(gp2x_queue)) / 4) 
#define GP2X_QUEUE_STRUCT_PTR          (0                  + 0x1000)
#define GP2X_QUEUE_DATA_PTR            (sizeof(gp2x_queue) + 0x1000)

#define gp2x_2ndcore_code(v)           (*(volatile unsigned long *)(v))
#define gp2x_1stcore_code(v)           gp2x_dualcore_ram[(v)>>2]
#define gp2x_2ndcore_data(v)           gp2x_2ndcore_code((v)+0x1ff6000)
#define gp2x_1stcore_data(v)           gp2x_1stcore_code((v)+0x1ff6000)

#define gp2x_2ndcore_code_ptr(v)       ((volatile unsigned long *)(v))
#define gp2x_1stcore_code_ptr(v)       (&gp2x_dualcore_ram[(v)>>2])
#define gp2x_2ndcore_data_ptr(v)       gp2x_2ndcore_code_ptr((v)+0x1ff6000)
#define gp2x_1stcore_data_ptr(v)       gp2x_1stcore_code_ptr((v)+0x1ff6000)

#define gp2x_dualcore_data(v)          gp2x_1stcore_data((v)<<2)

#define gp2x_dualcore_declare_subprogram(name) extern void gp2x_dualcore_launch_## name ##_subprogram(void);
#define gp2x_dualcore_launch_subprogram(name)  gp2x_dualcore_launch_## name ##_subprogram()




#ifdef MINIMAL_940T

#undef    gp2x_dualcore_data
#define   gp2x_dualcore_data(v)         gp2x_2ndcore_data((v)<<2)

#define   main                          gp2x_2ndcore_run

static void gp2x_2ndcore_start(void) __attribute__((naked));

static void gp2x_2ndcore_start(void)
{
  unsigned long gp2x_dequeue(gp2x_queue *q)
  {
   unsigned long data;

   while(!q->items); //waiting for head to increase...

   data=q->place940t[q->tail = (q->tail < q->max_items ? q->tail+1 : 0)];
   q->items--;

   return data;
  }

#define gp2x_dualcore_name_subprogram(name) \
/* 00000020 <_start>:                                                                                   */ \
/*    0:*/  asm volatile (".word 0xe59ff02c");  /*        ldr     pc, [pc, #44]  ; 34 <ioffset>         */ \
/*    4:*/  asm volatile (".word 0xe59ff028");  /*        ldr     pc, [pc, #40]  ; 34 <ioffset>         */ \
/*    8:*/  asm volatile (".word 0xe59ff024");  /*        ldr     pc, [pc, #36]  ; 34 <ioffset>         */ \
/*    c:*/  asm volatile (".word 0xe59ff020");  /*        ldr     pc, [pc, #32]  ; 34 <ioffset>         */ \
/*   10:*/  asm volatile (".word 0xe59ff01c");  /*        ldr     pc, [pc, #28]  ; 34 <ioffset>         */ \
/*   14:*/  asm volatile (".word 0xe59ff018");  /*        ldr     pc, [pc, #24]  ; 34 <ioffset>         */ \
/*   18:*/  asm volatile (".word 0xe59ff014");  /*        ldr     pc, [pc, #20]  ; 34 <ioffset>         */ \
/*   1c:*/  asm volatile (".word 0xe59ff010");  /*        ldr     pc, [pc, #16]  ; 34 <ioffset>         */ \
/* 00000020 <_init>:                                                                                    */ \
/*   20:*/  asm volatile (".word 0xe59fd010");  /*        ldr     sp, [pc, #16]   ; 38 <stack>          */ \
/*   24:*/  asm volatile (".word 0xe59fc010");  /*        ldr     ip, [pc, #16]   ; 3c <deadbeef>       */ \
/*   28:*/  asm volatile (".word 0xe59fb010");  /*        ldr     fp, [pc, #16]   ; 40 <leetface>       */ \
/*   2c:*/  asm volatile (".word 0xe92d1800");  /*        stmdb   sp!, {fp, ip}                         */ \
/*   30:*/  asm volatile (".word 0xea000004");  /*        b       48 <realinit>                         */ \
/* 00000034 <ioffset>:                                                                                  */ \
/*   34:*/  asm volatile (".word 0x00000020");  /*                                                      */ \
/* 00000038 <stack>:                                                                                    */ \
/*   38:*/  asm volatile (".word 0x01fffffc");  /*                                                      */ \
/* 0000003c <deadbeef>:                                                                                 */ \
/*   3c:*/  asm volatile (".word 0xdeadbeef");  /*                                                      */ \
/* 00000040 <leetface>:                                                                                 */ \
/*   40:*/  asm volatile (".word 0x1ee7face");  /*                                                      */ \
/* 00000044 <area1>:                                                                                    */ \
/*   44:*/  asm volatile (".word 0x01ff6019");  /*                                                      */ \
/* 00000048 <realinit>:                                                                                 */ \
/*  our main code starts here...                                                                        */ \
/*   48:*/  asm volatile (".word 0xe3a0003f");  /*        mov     r0, #63 ; 0x3f                        */ \
/*   4c:*/  asm volatile (".word 0xee060f10");  /*        mcr     15, 0, r0, cr6, cr0, {0}              */ \
/*   50:*/  asm volatile (".word 0xee060f30");  /*        mcr     15, 0, r0, cr6, cr0, {1}              */ \
/*   54:*/  asm volatile (".word 0xe3a0001f");  /*        mov     r0, #31 ; 0x1f                        */ \
/*   58:*/  asm volatile (".word 0xe38004be");  /*        orr     r0, r0, #-1107296256    ; 0xbe000000  */ \
/*   5c:*/  asm volatile (".word 0xee060f11");  /*        mcr     15, 0, r0, cr6, cr1, {0}              */ \
/*   60:*/  asm volatile (".word 0xe51f0024");  /*        ldr     r0, [pc, #-36]  ; 44 <area>           */ \
/*   64:*/  asm volatile (".word 0xee060f12");  /*        mcr     15, 0, r0, cr6, cr2, {0}              */ \
/*   68:*/  asm volatile (".word 0xe3a00001");  /*        mov     r0, #1  ; 0x1                         */ \
/*   6c:*/  asm volatile (".word 0xee020f10");  /*        mcr     15, 0, r0, cr2, cr0, {0}              */ \
/*   70:*/  asm volatile (".word 0xe3a00001");  /*        mov     r0, #1  ; 0x1                         */ \
/*   74:*/  asm volatile (".word 0xee020f30");  /*        mcr     15, 0, r0, cr2, cr0, {1}              */ \
/*   78:*/  asm volatile (".word 0xe3a00001");  /*        mov     r0, #1  ; 0x1                         */ \
/*   7c:*/  asm volatile (".word 0xee030f10");  /*        mcr     15, 0, r0, cr3, cr0, {0}              */ \
/*   80:*/  asm volatile (".word 0xe3a000ff");  /*        mov     r0, #255        ; 0xff                */ \
/*   84:*/  asm volatile (".word 0xee050f10");  /*        mcr     15, 0, r0, cr5, cr0, {0}              */ \
/*   88:*/  asm volatile (".word 0xe3a00003");  /*        mov     r0, #3  ; 0x3                         */ \
/*   8c:*/  asm volatile (".word 0xee050f30");  /*        mcr     15, 0, r0, cr5, cr0, {1}              */ \
/*   90:*/  asm volatile (".word 0xe3a00000");  /*        mov     r0, #0  ; 0x0                         */ \
/*   94:*/  asm volatile (".word 0xe3800001");  /*        orr     r0, r0, #1      ; 0x1                 */ \
/*   98:*/  asm volatile (".word 0xe380007c");  /*        orr     r0, r0, #124    ; 0x7c                */ \
/*   9c:*/  asm volatile (".word 0xe3800a01");  /*        orr     r0, r0, #4096   ; 0x1000              */ \
/*   a0:*/  asm volatile (".word 0xe3800103");  /*        orr     r0, r0, #-1073741824    ; 0xc0000000  */ \
/*   a4:*/  asm volatile (".word 0xee010f10");  /*        mcr     15, 0, r0, cr1, cr0, {0}              */ \
   asm volatile ("mcr p15, 0, r0, c7, c10, 4" ::: "r0"); \
   while(1) gp2x_2ndcore_run(gp2x_dequeue((gp2x_queue *)gp2x_2ndcore_data_ptr(GP2X_QUEUE_STRUCT_PTR))); \
} \
void gp2x_dualcore_launch_##name##_subprogram(void) { gp2x_dualcore_launch_program((unsigned long *)&gp2x_2ndcore_start, ((int)&gp2x_dualcore_launch_##name##_subprogram)-((int)&gp2x_2ndcore_start)); }

#endif


#if 0

@ 940Tcrt0.s for the fake-crt0s naked boombastic code in minimal_940t.h
@ Written by rlyeh, (c) 2005-2006.


      .section".init"
      .code 32
      .align
      .global _start
 
_start:
      LDR PC, [pc, #44]              @ Reset
      LDR PC, [pc, #40]              @ Undefined instruction
      LDR PC, [pc, #36]              @ Software Interrupt
      LDR PC, [pc, #32]              @ Prefetch abort
      LDR PC, [pc, #28]              @ Data abort
      LDR PC, [pc, #24]              @ Reserved
      LDR PC, [pc, #20]              @ IRQ
      LDR PC, [pc, #16]              @ FIQ
 
_init:
      LDR   SP, stack                @ Load our stack at top of 64mb (32mb from 940t)
      LDR   IP, deadbeef             @ Load deadbeef
      LDR   FP, leetface             @ Load face0000
      STMDB R13!, {R11,R12}
      .word 0xea000004               @ B realinit   
                                     @ putting B instead of .word may causes some troubles
                                     @ in *our* specific circumstances.
ioffset:
      .word  0x00000020
stack:          
      .word  0x01fffffc
deadbeef:
      .word  0xdeadbeef              @ Squidge's legacy :-)
leetface:
      .word  0x1ee7face

area:
      .word  0x01ff6019              @ at 920t's relative offset 0x4000000-0x8000(stack)-0x2000(shared area, where I map to)
                                     @ 00000011000b = 8kb | 00000000001b = enable
 
realinit:
      MOV R0, #0x3F                  @ 4Gb page starting at 0x000000
      MCR p15, 0, R0,c6,c0, 0        @ Set as data memory region 0
      MCR p15, 0, R0,c6,c0, 1        @ Set as instruction memory region 0

      MOV R0, #0x1F                  @ Region 1 for MP registers, 0xc0000000-0x2000000
      ORR R0, R0, #0xBE000000        @ Base at registers start
      MCR p15, 0, r0,c6,c1, 0        @ Set as data memory region 1

      LDR R0, area                   @ 8kb page. This is our dualcore FIFO queue
      MCR p15, 0, R0,c6,c2, 0        @ Set as data memory region 2
     @MCR p15, 0, R0,c6,c2, 1        @ Set as instruction memory region 2

     @Region 0 is cacheable/bufferable
     @Region 1 is uncacheable/unbufferable
     @Region 2 is uncacheable/bufferable

      MOV R0, #0x01                  @ Region 0 is cachable/bufferable
      MCR p15, 0, R0,c2,c0, 0        @ Write data cachable bits
      MOV R0, #0x01                   
      MCR p15, 0, R0,c2,c0, 1        @ Write instruction cachable bits
      MOV R0, #0x01                   
      MCR p15, 0, R0,c3,c0, 0        @ Write bufferable bits

      MOV R0, #0xFF                  @ Full access in all areas (read+write)
      MCR p15, 0, R0,c5,c0, 0        @ Write dataspace access permissions
      MOV R0, #0x03                  @ Only region 1 has inst permissions
      MCR p15, 0, R0,c5,c0, 1        @ Write inst space access permissions to region 1

      MOV R0, #0x00
     @MRC p15, 0, R0,c1,c0, 0        @ Read control register
      ORR R0, R0, #1                 @ Protection unit enabled
      ORR R0, R0, #0x7C              @ DCache enabled (should be #0x4?)
      ORR R0, R0, #0x1000            @ ICache enabled
      ORR R0, R0, #0xC0000000        @ Async clocking + FastBus
      MCR p15, 0, R0,c1,c0, 0        @ Set control register

.pool
.end


@Improvement:
@Maybe I should enable cache at shared area, and flush cache
@before checking/droping dualcore messages.

@some tools for this:
@MOV R0, #0
@MCR p15, 0, R0, c7, c5,  0 @Flush ICache
@MCR p15, 0, R0, c7, c6,  0 @Flush DCache
@MCR p15, 0, R0, c7, c10, 4 @Drain write buffer
@MOV R0, address to prefetch
@MCR p15, 0, R0, c7, c13, 1 @ICache prefetch


#endif

#endif
