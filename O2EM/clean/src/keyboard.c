/*
 *   O2EM Free Odyssey2 / Videopac+ Emulator
 *
 *   Created by Daniel Boris <dboris@comcast.net>  (c) 1997,1998
 *
 *   Developed by Andre de la Rocha   <adlroc@users.sourceforge.net>
 *             Arlindo M. de Oliveira <dgtec@users.sourceforge.net>
 *
 *   http://o2em.sourceforge.net
 *
 *
 *
 *   Keyboard emulation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cpu.h"
#include "config.h"
#include "vmachine.h"
#include "vdc.h"
#include "audio.h"
#include "voice.h"
#include "vpp.h"
#include "keyboard.h"
#include "allegro.h"
#include "score.h"

int NeedsPoll = 0;

Byte keycode;
Byte last_key;
Byte new_int=0;    /* Is new interrupt installed */

Byte key_done=0;
Byte key_debug=0;
#ifdef OPENEMU
struct keyb keybtab[] = {
    {KEY_A,"A"},
    {KEY_B,"B"},
    {KEY_C,"C"},
    {KEY_D,"D"},
    {KEY_E,"E"},
    {KEY_F,"F"},
    {KEY_G,"G"},
    {KEY_H,"H"},
    {KEY_I,"I"},
    {KEY_J,"J"},
    {KEY_K,"K"},
    {KEY_L,"L"},
    {KEY_M,"M"},
    {KEY_N,"N"},
    {KEY_O,"O"},
    {KEY_P,"P"},
    {KEY_Q,"Q"},
    {KEY_R,"R"},
    {KEY_S,"S"},
    {KEY_T,"T"},
    {KEY_U,"U"},
    {KEY_V,"V"},
    {KEY_W,"W"},
    {KEY_X,"X"},
    {KEY_Y,"Y"},
    {KEY_Z,"Z"},
    {KEY_0,"0"},
    {KEY_1,"1"},
    {KEY_2,"2"},
    {KEY_3,"3"},
    {KEY_4,"4"},
    {KEY_5,"5"},
    {KEY_6,"6"},
    {KEY_7,"7"},
    {KEY_8,"8"},
    {KEY_9,"9"},
    {KEY_0_PAD,"0_PAD"},
    {KEY_1_PAD,"1_PAD"},
    {KEY_2_PAD,"2_PAD"},
    {KEY_3_PAD,"3_PAD"},
    {KEY_4_PAD,"4_PAD"},
    {KEY_5_PAD,"5_PAD"},
    {KEY_6_PAD,"6_PAD"},
    {KEY_7_PAD,"7_PAD"},
    {KEY_8_PAD,"8_PAD"},
    {KEY_9_PAD,"9_PAD"},
    {KEY_TILDE,"TILDE"},
    {KEY_MINUS,"MINUS"},
    {KEY_EQUALS,"EQUALS"},
    {KEY_BACKSPACE,"BACKSPACE"},
    {KEY_TAB,"TAB"},
    {KEY_OPENBRACE,"OPENBRACE"},
    {KEY_CLOSEBRACE,"CLOSEBRACE"},
    {KEY_ENTER,"ENTER"},
    {KEY_COLON,"COLON"},
    {KEY_QUOTE,"QUOTE"},
    {KEY_BACKSLASH,"BACKSLASH"},
    {KEY_BACKSLASH2,"BACKSLASH2"},
    {KEY_COMMA,"COMMA"},
    {KEY_STOP,"STOP"},
    {KEY_SLASH,"SLASH"},
    {KEY_SPACE,"SPACE"},
    {KEY_INSERT,"INSERT"},
    {KEY_DEL,"DEL"},
    {KEY_HOME,"HOME"},
    {KEY_END,"END"},
    {KEY_PGUP,"PGUP"},
    {KEY_PGDN,"PGDN"},
    {KEY_LEFT,"LEFT"},
    {KEY_RIGHT,"RIGHT"},
    {KEY_UP,"UP"},
    {KEY_DOWN,"DOWN"},
    {KEY_SLASH_PAD,"SLASH_PAD"},
    {KEY_ASTERISK,"ASTERISK"},
    {KEY_MINUS_PAD,"MINUS_PAD"},
    {KEY_PLUS_PAD,"PLUS_PAD"},
    {KEY_DEL_PAD,"DEL_PAD"},
    {KEY_ENTER_PAD,"ENTER_PAD"},
    {KEY_PRTSCR,"PRTSCR"},
    {KEY_PAUSE,"PAUSE"},
    {KEY_ABNT_C1,"ABNT_C1"},
    {KEY_YEN,"YEN"},
    {KEY_KANA,"KANA"},
    {KEY_AT,"AT"},
    {KEY_CIRCUMFLEX,"CIRCUMFLEX"},
    {KEY_COLON2,"COLON2"},
    {KEY_KANJI,"KANJI"},
    {KEY_LSHIFT,"LSHIFT"},
    {KEY_RSHIFT,"RSHIFT"},
    {KEY_LCONTROL,"LCONTROL"},
    {KEY_RCONTROL,"RCONTROL"},
    {KEY_ALT,"ALT"},
    {KEY_ALTGR,"ALTGR"},
    {KEY_LWIN,"LWIN"},
    {KEY_RWIN,"RWIN"},
    {KEY_MENU,"MENU"},
    {KEY_SCRLOCK,"SCRLOCK"},
    {KEY_NUMLOCK,"NUMLOCK"},
    {KEY_F1,"F1"},
    {KEY_F2,"F2"},
    {KEY_F3,"F3"},
    {KEY_F4,"F4"},
    {KEY_F5,"F5"},
    {KEY_F6,"F6"},
    {KEY_F7,"F7"},
    {KEY_F8,"F8"},
    {KEY_F9,"F9"},
    {KEY_F10,"F10"},
    {KEY_F11,"F11"},
    {KEY_F12,"F12"},
    {KEY_ESC,"ESC"},
    {0,""}
};
#endif

int joykeys[2][5] = {{0,0,0,0,0},{0,0,0,0,0}};
int joykeystab[128];
int syskeys[8] = {0,0,0,0,0,0,0,0};

void set_defjoykeys(int stick_number, int control_scheme){
    printf("Setting keyboard controller %d to %d\n", stick_number, control_scheme);
    if (control_scheme) {
        // TODO : Implement Azery / Qwerty handler
        set_joykeys(stick_number, KEY_W, KEY_S, KEY_A, KEY_D, KEY_SPACE);
    } else {
        set_joykeys(stick_number,KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,KEY_L);
    }
}

void set_defsystemkeys(void)
{
    set_systemkeys(KEY_F12,KEY_F1,KEY_F4,KEY_F5,KEY_F8,KEY_F2,KEY_F3,KEY_F6);
}

void set_joykeys(int stick_number, int up, int down, int left, int right, int fire){
    printf("Setting joypad controller %d \n", stick_number);
    int i, j;
    if ((stick_number<0) || (stick_number>1)) {
        return;
    }
    joykeys[stick_number][0] = up;
    joykeys[stick_number][1] = down;
    joykeys[stick_number][2] = left;
    joykeys[stick_number][3] = right;
    joykeys[stick_number][4] = fire;

    for (i=0; i<128; i++) {
        joykeystab[i]=0;
    }

    for (j=0; j<2; j++) {
        for (i=0; i<5; i++) {
            if ((joykeys[j][i]<1) || (joykeys[j][i]>127)) {
                joykeys[j][i] = 0;
            } else {
                joykeystab[joykeys[j][i]] = 1;
            }
        }
    }
}

void set_systemkeys(int k_quit,int k_pause,int k_debug,int k_reset,int k_screencap,int k_save,int k_load,int k_inject)
{
    syskeys[0] = k_quit;
    syskeys[1] = k_pause;
    syskeys[2] = k_debug;
    syskeys[3] = k_reset;
    syskeys[4] = k_screencap;
    syskeys[5] = k_save;
    syskeys[6] = k_load;
    syskeys[7] = k_inject;
}

void handle_key(void){
    if (NeedsPoll) poll_keyboard();


    if (key[syskeys[0]] || key[KEY_ESC]) {
        do {
            rest(5);
            if (NeedsPoll) poll_keyboard();
        } while (key[syskeys[0]] || key[KEY_ESC]);
        key_done=1;
    }

    if (key[syskeys[1]]) {
        do {
            rest(5);
            if (NeedsPoll) poll_keyboard();
        } while (key[syskeys[1]]);

        mute_audio();
        mute_voice();
        abaut();

        do {
            rest(5);
            if (NeedsPoll) poll_keyboard();

            if (key[KEY_ALT] && key[KEY_ENTER]) {
                app_data.fullscreen = app_data.fullscreen ? 0 : 1;
                grmode();
                abaut();
                do {
                    rest(5);
                    if (NeedsPoll) poll_keyboard();
                } while (key[KEY_ENTER]);
            }

        } while ((!key[syskeys[1]]) && (!key[KEY_ESC]) && (!key[syskeys[0]]));
        do {
            rest(5);
            if (NeedsPoll) poll_keyboard();
        } while (key[syskeys[1]]);

        init_sound_stream();
    }

    if (key[syskeys[5]]) {
        if (savestate(app_data.statefile)==0) {
            display_msg("Savefile saved.",5);
        }
        do {
            rest(5);
            if (NeedsPoll) poll_keyboard();
        } while (key[syskeys[5]]);
    }

    /* LOAD STATE */
    if (key[syskeys[6]]) {
        int stateError;
        if ((stateError=loadstate(app_data.statefile))==0) {
            display_msg("Savefile loaded.",5);
        } else if (stateError>=199) {
            if (stateError==199) display_msg("Wrong ROM-File for Savefile.",5);
            else if (stateError==200+ROM_O2) display_msg("Wrong BIOS for Savefile: O2ROM needed.",5);
            else if (stateError==200+ROM_G7400) display_msg("Wrong BIOS for Savefile: G7400 ROM needed.",5);
            else if (stateError==200+ROM_C52) display_msg("Wrong BIOS for Savefile: C52 ROM needed.",5);
            else if (stateError==200+ROM_JOPAC) display_msg("Wrong BIOS for Savefile: JOPAC ROM needed.",5);
            else display_msg("Wrong BIOS for Savefile: UNKNOWN ROM needed.",5);
        }
        do {
            rest(5);
            if (NeedsPoll) poll_keyboard();
        } while (key[syskeys[6]]);
    }

    if (key[syskeys[2]]) key_debug=1;

    if (key[syskeys[3]]) {
        init_cpu();
        init_roms();
        init_vpp();
        clearscr();
        do {
            rest(5);
            if (NeedsPoll) poll_keyboard();
        } while (key[syskeys[3]]);
    }

    /* SET HIGHSCORE */
    if (key[syskeys[7]]) {
        set_score(app_data.scoretype, app_data.scoreaddress, app_data.default_highscore);
    }


    if (key[syskeys[4]]) {
#ifdef OPENEMU
        BITMAP *bmp;
        PALETTE pal;
        char *p;
        static char name[1024];
        static int scshot_counter = 0;

        if (strlen(app_data.scshot)>0){
            if ((p=strchr(app_data.scshot,'@'))) {
                *p = 0;
                sprintf(name, "%s%02d%s", app_data.scshot, scshot_counter++, p+1);
                *p = '@';
            } else {
                strcpy(name, app_data.scshot);
            }
            get_palette(pal);
            bmp = create_sub_bitmap(screen, 0, 0, SCREEN_W, SCREEN_H);
            save_bitmap(name, bmp, pal);
            destroy_bitmap(bmp);
            do {
                rest(5);
                if (NeedsPoll) poll_keyboard();
            } while (key[syskeys[4]]);
        }
#endif
    }

    if (key[KEY_ALT] && key[KEY_ENTER]) {
        app_data.fullscreen = app_data.fullscreen ? 0 : 1;
        grmode();
        do {
            rest(5);
            if (NeedsPoll) poll_keyboard();
        } while (key[KEY_ENTER]);
    }

}

Byte keyjoy(int jn){
    Byte d;
    d=0xFF;
    if ((jn>=0) && (jn<=1)){
        if (NeedsPoll) poll_keyboard();
        if (key[joykeys[jn][0]]) d &= 0xFE;
        if (key[joykeys[jn][1]]) d &= 0xFB;
        if (key[joykeys[jn][2]]) d &= 0xF7;
        if (key[joykeys[jn][3]]) d &= 0xFD;
        if (key[joykeys[jn][4]]) d &= 0xEF;
    }
    return d;
}


void init_keyboard(void){
    key_done=0;
    key_debug=0;
    install_keyboard();
    new_int=1;
    NeedsPoll = keyboard_needs_poll();
}


void Set_Old_Int9(void){
   remove_keyboard();
   new_int=0;
}

