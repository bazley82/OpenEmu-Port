
#ifndef ALLEGRO_H
#define ALLEGRO_H

#define INLINE static inline
#define rest(a) usleep(a)
#define strupr upcase
#define strlwr downcase
#define keypressed() 0
#define poll_keyboard()
#define yield_timeslice()

typedef struct
{  
    unsigned char *line;
    int w;
    int h;
    int pitch;
    int depth;
    
}BITMAP;

//typedef struct
//{
//    unsigned char r;
//    unsigned char g;
//    unsigned char b;
//}APALETTE;

typedef struct RGB
{
    unsigned char r, g, b;
    unsigned char filler;
} RGB;
#define PAL_SIZE 256
typedef RGB PALETTE[PAL_SIZE];

struct FONT_VTABLE;
typedef struct FONT
{
    void *data;
    int height;
    struct FONT_VTABLE *vtable;
} FONT;

typedef struct AUDIOSTREAM
{
} AUDIOSTREAM;

typedef struct DIGI_DRIVER /* driver for playing digital sfx */
{
} DIGI_DRIVER;

#define DIGI_AUTODETECT -1 /* for passing to install_sound() */
#define MIDI_NONE 0

#define GFX_TEXT -1
#define GFX_AUTODETECT 0
#define GFX_AUTODETECT_FULLSCREEN 1
#define GFX_AUTODETECT_WINDOWED 2
#define SWITCH_PAUSE 1
extern BITMAP * screen;
void set_color_depth (int depth);
int set_gfx_mode (int card, int w, int h, int v_w, int v_h);
void set_palette (const PALETTE p);
void set_window_title (const char *name);
int set_display_switch_mode (int mode);
int set_gfx_mode (int card, int w, int h, int v_w, int v_h);
void acquire_screen (void);
void release_screen (void);
void clear (BITMAP *bmp);

void stretch_blit (BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h);

void textprintf_ex (BITMAP *bmp, const FONT *f, int x, int y, int color, int bg, const char *format, ...);

extern unsigned char key[256*2];

#define INLINE static inline

#endif
