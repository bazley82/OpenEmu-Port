
#include "HandController.h"
#include "Intellivision.h"
#include <math.h>

// TODO: jeremiah sypult cross-platform
// classic game controller support should be handled in the input implementation
#if defined( CLASSIC_GAME_CONTROLLER )
#include "cgc.h"
#endif /* CLASSIC_GAME_CONTROLLER */

const double PI = 3.14159265358979323846264338327950288;

const float HandController::vectorParse = (float)sin(PI/4.0);

const UINT16 HandController::BUTTON_OUTPUT_VALUES[15] = {
    0x81, //OUTPUT_KEYPAD_ONE
    0x41, //OUTPUT_KEYPAD_TWO
    0x21, //OUTPUT_KEYPAD_THREE
    0x82, //OUTPUT_KEYPAD_FOUR
    0x42, //OUTPUT_KEYPAD_FIVE
    0x22, //OUTPUT_KEYPAD_SIX
    0x84, //OUTPUT_KEYPAD_SEVEN
    0x44, //OUTPUT_KEYPAD_EIGHT
    0x24, //OUTPUT_KEYPAD_NINE
    0x88, //OUTPUT_KEYPAD_CLEAR
    0x48, //OUTPUT_KEYPAD_ZERO
    0x28, //OUTPUT_KEYPAD_ENTER
    0xA0, //OUTPUT_ACTION_BUTTON_TOP
    0x60, //OUTPUT_ACTION_BUTTON_BOTTOM_LEFT
    0xC0  //OUTPUT_ACTION_BUTTON_BOTTOM_RIGHT
};

const UINT16 HandController::DIRECTION_OUTPUT_VALUES[16] = {
    0x04, //OUTPUT_DISC_NORTH
    0x14, //OUTPUT_DISC_NORTH_NORTH_EAST
    0x16, //OUTPUT_DISC_NORTH_EAST
    0x06, //OUTPUT_DISC_EAST_NORTH_EAST
    0x02, //OUTPUT_DISC_EAST
    0x12, //OUTPUT_DISC_EAST_SOUTH_EAST
    0x13, //OUTPUT_DISC_SOUTH_EAST
    0x03, //OUTPUT_DISC_SOUTH_SOUTH_EAST
    0x01, //OUTPUT_DISC_SOUTH
    0x11, //OUTPUT_DISC_SOUTH_SOUTH_WEST
    0x19, //OUTPUT_DISC_SOUTH_WEST
    0x09, //OUTPUT_DISC_WEST_SOUTH_WEST
    0x08, //OUTPUT_DISC_WEST
    0x18, //OUTPUT_DISC_WEST_NORTH_WEST
    0x1C, //OUTPUT_DISC_NORTH_WEST
    0x0C  //OUTPUT_DISC_NORTH_NORTH_WEST
};

HandController::HandController(INT32 id, const CHAR* n)
: InputConsumer(id),
  name(n)
{
    inputConsumerObjects[0] = new InputConsumerObject(0, "Keypad 1", CONTROLLER_KEYPAD_1);
    inputConsumerObjects[1] = new InputConsumerObject(1, "Keypad 2", CONTROLLER_KEYPAD_2);
    inputConsumerObjects[2] = new InputConsumerObject(2, "Keypad 3", CONTROLLER_KEYPAD_3);
    inputConsumerObjects[3] = new InputConsumerObject(3, "Keypad 4", CONTROLLER_KEYPAD_4);
    inputConsumerObjects[4] = new InputConsumerObject(4, "Keypad 5", CONTROLLER_KEYPAD_5);
    inputConsumerObjects[5] = new InputConsumerObject(5, "Keypad 6", CONTROLLER_KEYPAD_6);
    inputConsumerObjects[6] = new InputConsumerObject(6, "Keypad 7", CONTROLLER_KEYPAD_7);
    inputConsumerObjects[7] = new InputConsumerObject(7, "Keypad 8", CONTROLLER_KEYPAD_8);
    inputConsumerObjects[8] = new InputConsumerObject(8, "Keypad 9", CONTROLLER_KEYPAD_9);
    inputConsumerObjects[9] = new InputConsumerObject(9, "Keypad CLEAR", CONTROLLER_KEYPAD_CLEAR);
    inputConsumerObjects[10] = new InputConsumerObject(10, "Keypad 0", CONTROLLER_KEYPAD_0);
    inputConsumerObjects[11] = new InputConsumerObject(11, "Keypad ENTER", CONTROLLER_KEYPAD_ENTER);
    inputConsumerObjects[12] = new InputConsumerObject(12, "Top Action Buttons", CONTROLLER_ACTION_TOP);
    inputConsumerObjects[13] = new InputConsumerObject(13, "Bottom Left Action Button", CONTROLLER_ACTION_BOTTOM_LEFT);
    inputConsumerObjects[14] = new InputConsumerObject(14, "Bottom Right Action Button", CONTROLLER_ACTION_BOTTOM_RIGHT);
    inputConsumerObjects[15] = new InputConsumerObject(15, "Disc Pad Up", CONTROLLER_DISC_UP);
    inputConsumerObjects[16] = new InputConsumerObject(16, "Disc Pad Up-Right", CONTROLLER_DISC_UP_RIGHT);
    inputConsumerObjects[17] = new InputConsumerObject(17, "Disc Pad Right", CONTROLLER_DISC_RIGHT);
    inputConsumerObjects[18] = new InputConsumerObject(18, "Disc Pad Down-Right", CONTROLLER_DISC_DOWN_RIGHT);
    inputConsumerObjects[19] = new InputConsumerObject(19, "Disc Pad Down", CONTROLLER_DISC_DOWN);
    inputConsumerObjects[20] = new InputConsumerObject(20, "Disc Pad Down-Left", CONTROLLER_DISC_DOWN_LEFT);
    inputConsumerObjects[21] = new InputConsumerObject(21, "Disc Pad Left", CONTROLLER_DISC_LEFT);
    inputConsumerObjects[22] = new InputConsumerObject(22, "Disc Pad Up-Left", CONTROLLER_DISC_UP_LEFT);
}

HandController::~HandController()
{
    for (int i = 0; i < NUM_HAND_CONTROLLER_OBJECTS; i++)
        delete inputConsumerObjects[i];
}

INT32 HandController::getInputConsumerObjectCount()
{
    return NUM_HAND_CONTROLLER_OBJECTS;
}

InputConsumerObject* HandController::getInputConsumerObject(INT32 i)
{
    return inputConsumerObjects[i];
}

void HandController::evaluateInputs()
{
    static const float offset = (2.0f * PI)/16.0f;

    inputValue = 0;

    for (UINT16 i = 0; i < 16; i++) {
        if (inputConsumerObjects[i]->getInputValue() == 1.0f)
            inputValue |= BUTTON_OUTPUT_VALUES[i];
    }

    //evaluate the disc
    float neswVector =
        (inputConsumerObjects[16]->getInputValue() -
        inputConsumerObjects[20]->getInputValue()) *
        vectorParse;
    float nwseVector =
        (inputConsumerObjects[22]->getInputValue() -
        inputConsumerObjects[18]->getInputValue()) *
        vectorParse;
    float yPos = inputConsumerObjects[15]->getInputValue() -
        inputConsumerObjects[19]->getInputValue() +
        nwseVector + neswVector;
    float xPos = inputConsumerObjects[17]->getInputValue() -
        inputConsumerObjects[21]->getInputValue() -
        nwseVector + neswVector;

    if (xPos != 0 || yPos != 0) {
        float positionInRadians = (atan2f(-xPos, -yPos)+PI);
        UINT16 directionIndex = (UINT16)((positionInRadians+(offset/2.0))/offset) & 0x0F;
        UINT16 directionValue = DIRECTION_OUTPUT_VALUES[directionIndex];
        inputValue |= directionValue;
    }

#if defined( CLASSIC_GAME_CONTROLLER )
    if (usingCGC) {
        //also support the CGC
        USHORT intyData;
        L_CGCGetCookedIntyData(getId() == 0 ? CONTROLLER_0 : CONTROLLER_1, &intyData);

        //decode the CGC data
        if (intyData & 0x000F)
            inputValue |= BUTTON_OUTPUT_VALUES[(intyData & 0x000F)-1];
        if (intyData & 0x0010)
            inputValue |= BUTTON_OUTPUT_VALUES[12];
        if (intyData & 0x0020)
            inputValue |= BUTTON_OUTPUT_VALUES[13];
        if (intyData & 0x0040)
            inputValue |= BUTTON_OUTPUT_VALUES[14];
        if (intyData & 0x0080)
            inputValue |= DIRECTION_OUTPUT_VALUES[(intyData & 0x0F00) >> 8];
    }
#endif /* CLASSIC_GAME_CONTROLLER */
    inputValue = (UINT16)(0xFF ^ inputValue);
}

void HandController::resetInputConsumer()
{
    inputValue = 0xFF;
}

void HandController::setOutputValue(UINT16 value)
{
    inputValue = value;
}

UINT16 HandController::getInputValue()
{
    return inputValue;
}
