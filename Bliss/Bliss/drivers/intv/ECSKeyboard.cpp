
#include "ECSKeyboard.h"

const INT32 ECSKeyboard::sortedObjectIndices[NUM_ECS_OBJECTS] = {
    24, 29, 34, 28, 33, 27, 32, 26, 31, 25, 30, // ESCAPE, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0
    23, 22, 40, 21, 39, 20, 38, 19, 37, 18,     // Q, W, E, R, T, Y, U, I, O, P
    47, 16, 46, 15, 45, 14, 44, 13, 43, 12, 36, // A, S, D, F, G, H, J, K, L, SEMICOLON, ENTER
    10,  4,  9,  3,  8,  2,  7,  1,  6,         // Z, X, C, V, B, N, M, COMMA, PERIOD
    41, 42,  5,                                 // CTRL, SHIFT, SPACE,
    17, 11,  0,  35,                            // UP, DOWN, LEFT, RIGHT
};

ECSKeyboard::ECSKeyboard(INT32 id)
: InputConsumer(id),
  rowsToScan(0)
{
    memset(rowInputValues, 0, sizeof(rowInputValues));
    inputConsumerObjects[0] = new InputConsumerObject(0, "Left", ECS_KEYBOARD_LEFT);
    inputConsumerObjects[1] = new InputConsumerObject(1, "Comma", ECS_KEYBOARD_COMMA);
    inputConsumerObjects[2] = new InputConsumerObject(2, "N", ECS_KEYBOARD_N);
    inputConsumerObjects[3] = new InputConsumerObject(3, "V", ECS_KEYBOARD_V);
    inputConsumerObjects[4] = new InputConsumerObject(4, "X", ECS_KEYBOARD_X);
    inputConsumerObjects[5] = new InputConsumerObject(5, "Space", ECS_KEYBOARD_SPACE);
    inputConsumerObjects[6] = new InputConsumerObject(6, "Period", ECS_KEYBOARD_PERIOD);
    inputConsumerObjects[7] = new InputConsumerObject(7, "M", ECS_KEYBOARD_M);
    inputConsumerObjects[8] = new InputConsumerObject(8, "B", ECS_KEYBOARD_B);
    inputConsumerObjects[9] = new InputConsumerObject(9, "C", ECS_KEYBOARD_C);
    inputConsumerObjects[10] = new InputConsumerObject(10, "Z", ECS_KEYBOARD_Z);
    inputConsumerObjects[11] = new InputConsumerObject(11, "Down", ECS_KEYBOARD_DOWN);
    inputConsumerObjects[12] = new InputConsumerObject(12, "Semicolon", ECS_KEYBOARD_SEMICOLON);
    inputConsumerObjects[13] = new InputConsumerObject(13, "K", ECS_KEYBOARD_K);
    inputConsumerObjects[14] = new InputConsumerObject(14, "H", ECS_KEYBOARD_H);
    inputConsumerObjects[15] = new InputConsumerObject(15, "F", ECS_KEYBOARD_F);
    inputConsumerObjects[16] = new InputConsumerObject(16, "S", ECS_KEYBOARD_S);
    inputConsumerObjects[17] = new InputConsumerObject(17, "Up", ECS_KEYBOARD_UP);
    inputConsumerObjects[18] = new InputConsumerObject(18, "P", ECS_KEYBOARD_P);
    inputConsumerObjects[19] = new InputConsumerObject(19, "I", ECS_KEYBOARD_I);
    inputConsumerObjects[20] = new InputConsumerObject(20, "Y", ECS_KEYBOARD_Y);
    inputConsumerObjects[21] = new InputConsumerObject(21, "R", ECS_KEYBOARD_R);
    inputConsumerObjects[22] = new InputConsumerObject(22, "W", ECS_KEYBOARD_W);
    inputConsumerObjects[23] = new InputConsumerObject(23, "Q", ECS_KEYBOARD_Q);
    inputConsumerObjects[24] = new InputConsumerObject(24, "Escape", ECS_KEYBOARD_ESCAPE);
    inputConsumerObjects[25] = new InputConsumerObject(25, "9", ECS_KEYBOARD_9);
    inputConsumerObjects[26] = new InputConsumerObject(26, "7", ECS_KEYBOARD_7);
    inputConsumerObjects[27] = new InputConsumerObject(27, "5", ECS_KEYBOARD_5);
    inputConsumerObjects[28] = new InputConsumerObject(28, "3", ECS_KEYBOARD_3);
    inputConsumerObjects[29] = new InputConsumerObject(29, "1", ECS_KEYBOARD_1);
    inputConsumerObjects[30] = new InputConsumerObject(30, "0", ECS_KEYBOARD_0);
    inputConsumerObjects[31] = new InputConsumerObject(31, "8", ECS_KEYBOARD_8);
    inputConsumerObjects[32] = new InputConsumerObject(32, "6", ECS_KEYBOARD_6);
    inputConsumerObjects[33] = new InputConsumerObject(33, "4", ECS_KEYBOARD_4);
    inputConsumerObjects[34] = new InputConsumerObject(34, "2", ECS_KEYBOARD_2);
    inputConsumerObjects[35] = new InputConsumerObject(35, "Right", ECS_KEYBOARD_RIGHT);
    inputConsumerObjects[36] = new InputConsumerObject(36, "Return", ECS_KEYBOARD_RETURN);
    inputConsumerObjects[37] = new InputConsumerObject(37, "O", ECS_KEYBOARD_O);
    inputConsumerObjects[38] = new InputConsumerObject(38, "U", ECS_KEYBOARD_U);
    inputConsumerObjects[39] = new InputConsumerObject(39, "T", ECS_KEYBOARD_T);
    inputConsumerObjects[40] = new InputConsumerObject(40, "E", ECS_KEYBOARD_E);
    inputConsumerObjects[41] = new InputConsumerObject(41, "Control", ECS_KEYBOARD_CONTROL);
    inputConsumerObjects[42] = new InputConsumerObject(42, "Shift", ECS_KEYBOARD_SHIFT);
    inputConsumerObjects[43] = new InputConsumerObject(43, "L", ECS_KEYBOARD_L);
    inputConsumerObjects[44] = new InputConsumerObject(44, "J", ECS_KEYBOARD_J);
    inputConsumerObjects[45] = new InputConsumerObject(45, "G", ECS_KEYBOARD_G);
    inputConsumerObjects[46] = new InputConsumerObject(46, "D", ECS_KEYBOARD_D);
    inputConsumerObjects[47] = new InputConsumerObject(47, "A", ECS_KEYBOARD_A);
}

ECSKeyboard::~ECSKeyboard()
{
    for (int i = 0; i < NUM_ECS_OBJECTS; i++)
        delete inputConsumerObjects[i];
}

INT32 ECSKeyboard::getInputConsumerObjectCount()
{
    return NUM_ECS_OBJECTS;
}

InputConsumerObject* ECSKeyboard::getInputConsumerObject(INT32 i)
{
    INT32 sortedIndex = sortedObjectIndices[i];
    return inputConsumerObjects[sortedIndex];
}

void ECSKeyboard::resetInputConsumer()
{
    rowsToScan = 0;
    for (UINT16 i = 0; i < 8; i++)
        rowInputValues[i] = 0;
}

void ECSKeyboard::evaluateInputs()
{
    for (UINT16 row = 0; row < 6; row++) {
        rowInputValues[row] = 0;
        if (inputConsumerObjects[row]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x01;
        if (inputConsumerObjects[row+6]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x02;
        if (inputConsumerObjects[row+12]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x04;
        if (inputConsumerObjects[row+18]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x08;
        if (inputConsumerObjects[row+24]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x10;
        if (inputConsumerObjects[row+30]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x20;
        if (inputConsumerObjects[row+36]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x40;
        if (inputConsumerObjects[row+42]->getInputValue() == 1.0f)
            rowInputValues[row] |= 0x80;
    }
    rowInputValues[6] = (inputConsumerObjects[47]->getInputValue() == 1.0f ? 0x80 : 0);
}

UINT16 ECSKeyboard::getInputValue()
{
    UINT16 inputValue = 0;
    UINT16 rowMask = 1;
    for (UINT16 row = 0; row < 8; row++)  {
        if ((rowsToScan & rowMask) != 0) {
            rowMask = (UINT16)(rowMask << 1);
            continue;
        }
        inputValue |= rowInputValues[row];

        rowMask = (UINT16)(rowMask << 1);
    }

    inputValue = (UINT16)(0xFF ^ inputValue);

    return inputValue;
}

void ECSKeyboard::setOutputValue(UINT16 value)
{
    this->rowsToScan = value;
}
