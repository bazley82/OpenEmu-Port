#ifndef __KEYBOARD_H
#define __KEYBOARD_H

void Set_Old_Int9(void);
void init_keyboard(void);
void handle_key(void);
void set_joykeys(int joy, int up, int down, int left, int right, int fire);
void set_systemkeys(int k_quit, int k_pause, int k_debug, int k_reset,
                    int k_screencap, int k_save, int k_load, int k_inject);
void set_defjoykeys(int joy, int sc);
void set_defsystemkeys(void);
Byte keyjoy(int jn);

extern Byte new_int;
extern int NeedsPoll;
extern Byte key_done;
extern Byte key_debug;
extern int joykeys[2][5];
extern int joykeystab[128];
extern int syskeys[8];

struct keyb
{
    int keybcode;
    char *keybname;
};

extern struct keyb keybtab[];

#ifdef OPENEMU
#endif

#define KEY_W 0
#define KEY_S 0
#define KEY_A 0
#define KEY_D 0
#define KEY_SPACE 0
#define KEY_UP 0
#define KEY_DOWN 0
#define KEY_LEFT 0
#define KEY_RIGHT 0
#define KEY_L 0
#define KEY_F12 0
#define KEY_F1 0
#define KEY_F4 0
#define KEY_F5 0
#define KEY_F8 0
#define KEY_F2 0
#define KEY_F3 0
#define KEY_F6 0
#define KEY_ESC 0
#define KEY_ALT 0
#define KEY_ENTER 0

#define KEY_0 0
#define KEY_1 0
#define KEY_2 0
#define KEY_3 0
#define KEY_4 0
#define KEY_5 0
#define KEY_6 0
#define KEY_7 0
#define KEY_8 0
#define KEY_9 0
#define KEY_SLASH 0
#define KEY_P 0
#define KEY_PLUS_PAD 0
#define KEY_E 0
#define KEY_R 0
#define KEY_T 0
#define KEY_U 0
#define KEY_I 0
#define KEY_O 0
#define KEY_Q 0
#define KEY_F 0
#define KEY_G 0
#define KEY_H 0
#define KEY_J 0
#define KEY_K 0
#define KEY_Z 0
#define KEY_X 0
#define KEY_C 0
#define KEY_V 0
#define KEY_B 0
#define KEY_M 0
#define KEY_STOP 0
#define KEY_MINUS 0
#define KEY_ASTERISK 0
#define KEY_SLASH_PAD 0
#define KEY_EQUALS 0
#define KEY_Y 0
#define KEY_N 0
#define KEY_DEL 0

#endif

