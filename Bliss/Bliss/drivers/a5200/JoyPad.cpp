
#include "JoyPad.h"

// TODO: jeremiah sypult cross-platform
#if defined( _WIN32 )
#include "dinput.h"
#endif

const UINT8 JoyPad::KBCODES[15] = {
        0x0C, //Start   1100
        0x08, //Pause   1000
        0x04, //Reset   0100
        0x0F, //1       1111
        0x0E, //2       1110
        0x0D, //3       1101
        0x0B, //4       1011
        0x0A, //5       1010
        0x09, //6       1001
        0x07, //7       0111
        0x06, //8       0110
        0x05, //9       0101
        0x03, //*       0011
        0x02, //0       0010
        0x01, //#       0001
};

JoyPad::JoyPad(INT32 id, const CHAR* name)
: InputConsumer(id)
{
    this->name = new CHAR[strlen(name)+1];
    strcpy(this->name, name);

    inputObjects[0] = new InputConsumerObject(0, "Start", JOYPAD_START);
    inputObjects[1] = new InputConsumerObject(1, "Pause", JOYPAD_PAUSE);
    inputObjects[2] = new InputConsumerObject(2, "Reset", JOYPAD_RESET);
    inputObjects[3] = new InputConsumerObject(3, "Keypad 1", JOYPAD_KEYPAD_1);
    inputObjects[4] = new InputConsumerObject(4, "Keypad 2", JOYPAD_KEYPAD_2);
    inputObjects[5] = new InputConsumerObject(5, "Keypad 3", JOYPAD_KEYPAD_3);
    inputObjects[6] = new InputConsumerObject(6, "Keypad 4", JOYPAD_KEYPAD_4);
    inputObjects[7] = new InputConsumerObject(7, "Keypad 5", JOYPAD_KEYPAD_5);
    inputObjects[8] = new InputConsumerObject(8, "Keypad 6", JOYPAD_KEYPAD_6);
    inputObjects[9] = new InputConsumerObject(9, "Keypad 7", JOYPAD_KEYPAD_7);
    inputObjects[10] = new InputConsumerObject(10, "Keypad 8", JOYPAD_KEYPAD_8);
    inputObjects[11] = new InputConsumerObject(11, "Keypad 9", JOYPAD_KEYPAD_9);
    inputObjects[12] = new InputConsumerObject(12, "Keypad Star (*)", JOYPAD_KEYPAD_STAR);
    inputObjects[13] = new InputConsumerObject(13, "Keypad 0", JOYPAD_KEYPAD_0);
    inputObjects[14] = new InputConsumerObject(14, "Keypad Pound (#)", JOYPAD_KEYPAD_POUND);
    inputObjects[15] = new InputConsumerObject(15, "Top Action Buttons", JOYPAD_ACTION_TOP);
    inputObjects[16] = new InputConsumerObject(16, "Bottom Action Buttons", JOYPAD_ACTION_BOTTOM);
    inputObjects[17] = new InputConsumerObject(17, "JoyStick Up", JOYPAD_STICK_UP);
    inputObjects[18] = new InputConsumerObject(18, "JoyStick Right", JOYPAD_STICK_RIGHT);
    inputObjects[19] = new InputConsumerObject(19, "JoyStick Down", JOYPAD_STICK_DOWN);
    inputObjects[20] = new InputConsumerObject(20, "JoyStick Left", JOYPAD_STICK_LEFT);
}

JoyPad::~JoyPad()
{
    delete[] name;

    for (INT32 i = 0; i < 21; i++)
        delete inputObjects[i];
}

const CHAR* JoyPad::getName()
{
    return name;
}

void JoyPad::resetInputConsumer()
{
    kbcode = 0;
    potx = 0;
    poty = 0;
}

void JoyPad::evaluateInputs()
{
    //scan the keypad
    kbcode = 0;
    for (int i = 0; i < 15; i++) {
        if (inputObjects[i]->getInputValue() != 0.0f)
            kbcode |= KBCODES[i];
    }

    //top fire buttons
    /*
    if (inputObjects[15]->getInputValue() != 0.0f)
        kbcode |= 0x20;
    */

    //x direction
    potx = (UINT8)(114.0f - (inputObjects[20]->getInputValue() * 114.0f) + (inputObjects[18]->getInputValue() * 114.0f));
    //y direction
    poty = (UINT8)(114.0f - (inputObjects[17]->getInputValue() * 114.0f) + (inputObjects[19]->getInputValue() * 114.0f));
}

