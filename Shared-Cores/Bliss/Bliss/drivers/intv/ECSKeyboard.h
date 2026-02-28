
#ifndef ECSKEYBOARD_H
#define ECSKEYBOARD_H

#define NUM_ECS_OBJECTS 48

#include "core/audio/AY38914_InputOutput.h"
#include "core/input/InputConsumer.h"

class ECSKeyboard : public AY38914_InputOutput, public InputConsumer
{
    public:
        ECSKeyboard(INT32 id);
        virtual ~ECSKeyboard();

        const CHAR* getName() { return "ECS Keyboard"; }

        void resetInputConsumer();

        /**
         * Poll the controller.  This function is invoked by the
         * InputConsumerBus just after the Emulator indicates it has entered
         * vertical blank.
         */
        void evaluateInputs();

        //functions to get descriptive info about the input consumer
        INT32 getInputConsumerObjectCount();
        InputConsumerObject* getInputConsumerObject(INT32 i);

        UINT16 getInputValue();
        void setOutputValue(UINT16 value);

        static const INT32         sortedObjectIndices[NUM_ECS_OBJECTS];

    private:
        InputConsumerObject* inputConsumerObjects[NUM_ECS_OBJECTS];
        UINT16        rowsToScan;
        UINT16        rowInputValues[8];
};

// jeremiah sypult
enum
{
    ECS_KEYBOARD_LEFT = 0x00,
    ECS_KEYBOARD_COMMA = 0x01,
    ECS_KEYBOARD_N = 0x02,
    ECS_KEYBOARD_V = 0x03,
    ECS_KEYBOARD_X = 0x04,
    ECS_KEYBOARD_SPACE = 0x05,
    ECS_KEYBOARD_PERIOD = 0x06,
    ECS_KEYBOARD_M = 0x07,
    ECS_KEYBOARD_B = 0x08,
    ECS_KEYBOARD_C = 0x09,
    ECS_KEYBOARD_Z = 0x0A,
    ECS_KEYBOARD_DOWN = 0x0B,
    ECS_KEYBOARD_SEMICOLON = 0x0C,
    ECS_KEYBOARD_K = 0x0D,
    ECS_KEYBOARD_H = 0x0E,
    ECS_KEYBOARD_F = 0x0F,
    ECS_KEYBOARD_S = 0x10,
    ECS_KEYBOARD_UP = 0x11,
    ECS_KEYBOARD_P = 0x12,
    ECS_KEYBOARD_I = 0x13,
    ECS_KEYBOARD_Y = 0x14,
    ECS_KEYBOARD_R = 0x15,
    ECS_KEYBOARD_W = 0x16,
    ECS_KEYBOARD_Q = 0x17,
    ECS_KEYBOARD_ESCAPE = 0x18,
    ECS_KEYBOARD_9 = 0x19,
    ECS_KEYBOARD_7 = 0x1A,
    ECS_KEYBOARD_5 = 0x1B,
    ECS_KEYBOARD_3 = 0x1C,
    ECS_KEYBOARD_1 = 0x1D,
    ECS_KEYBOARD_0 = 0x1E,
    ECS_KEYBOARD_8 = 0x1F,
    ECS_KEYBOARD_6 = 0x20,
    ECS_KEYBOARD_4 = 0x21,
    ECS_KEYBOARD_2 = 0x22,
    ECS_KEYBOARD_RIGHT = 0x23,
    ECS_KEYBOARD_RETURN = 0x24,
    ECS_KEYBOARD_O = 0x25,
    ECS_KEYBOARD_U = 0x26,
    ECS_KEYBOARD_T = 0x27,
    ECS_KEYBOARD_E = 0x28,
    ECS_KEYBOARD_CONTROL = 0x29,
    ECS_KEYBOARD_SHIFT = 0x2A,
    ECS_KEYBOARD_L = 0x2B,
    ECS_KEYBOARD_J = 0x2C,
    ECS_KEYBOARD_G = 0x2D,
    ECS_KEYBOARD_D = 0x2E,
    ECS_KEYBOARD_A = 0x2F
};

#endif
