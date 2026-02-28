
#ifndef JOYPAD_H
#define JOYPAD_H

#include "core/input/InputConsumer.h"
#include "core/audio/Pokey_Input.h"

class JoyPad : public InputConsumer, public Pokey_Input
{

public:
    JoyPad(INT32 id, const CHAR* name);
    virtual ~JoyPad();

    const CHAR* getName();

    void resetInputConsumer();

    void evaluateInputs();

    INT32 getInputConsumerObjectCount() { return 21; }
    InputConsumerObject* getInputConsumerObject(INT32 i) { return inputObjects[i]; }

    UINT8 getKeyPadCode() { return kbcode; }
    UINT8 getPotX() { return potx; }
    UINT8 getPotY() { return poty; }

private:
    InputConsumerObject* inputObjects[21];
    
    CHAR* name;
    UINT8 kbcode;
    UINT8 potx;
    UINT8 poty;

    const static UINT8 KBCODES[15];
};

// jeremiah sypult
enum
{
    JOYPAD_START = 0x01,
    JOYPAD_PAUSE = 0x02,
    JOYPAD_RESET = 0x03,
    JOYPAD_KEYPAD_1 = 0x04,
    JOYPAD_KEYPAD_2 = 0x05,
    JOYPAD_KEYPAD_3 = 0x06,
    JOYPAD_KEYPAD_4 = 0x07,
    JOYPAD_KEYPAD_5 = 0x08,
    JOYPAD_KEYPAD_6 = 0x09,
    JOYPAD_KEYPAD_7 = 0x0A,
    JOYPAD_KEYPAD_8 = 0x0B,
    JOYPAD_KEYPAD_9 = 0x0C,
    JOYPAD_KEYPAD_STAR = 0x0D,
    JOYPAD_KEYPAD_0 = 0x0E,
    JOYPAD_KEYPAD_POUND = 0x0F,
    JOYPAD_ACTION_TOP = 0x10,
    JOYPAD_ACTION_BOTTOM = 0x11,
    JOYPAD_STICK_UP = 0x12,
    JOYPAD_STICK_RIGHT = 0x13,
    JOYPAD_STICK_DOWN = 0x14,
    JOYPAD_STICK_LEFT = 0x15
};

#endif
