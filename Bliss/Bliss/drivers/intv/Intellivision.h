
#ifndef INTELLIVISION_H
#define INTELLIVISION_H

#include "core/Emulator.h"
#include "HandController.h"
#include "ECS.h"
#include "Intellivoice.h"
#include "core/Emulator.h"
#include "core/memory/MemoryBus.h"
#include "core/memory/RAM.h"
#include "core/memory/ROM.h"
#include "core/cpu/CP1610.h"
#include "core/video/AY38900.h"
#include "core/audio/AY38914.h"

#define RAM8BIT_SIZE    0x00F0
#define RAM16BIT_SIZE   0x0160

TYPEDEF_STRUCT_PACK( _IntellivisionState
{
    StateHeader          header;
    StateChunk           cpu;
    CP1610State          cpuState;
    StateChunk           stic;
    AY38900State         sticState;
    StateChunk           psg;
    AY38914State         psgState;
    StateChunk           RAM8bit;
    RAMState             RAM8bitState;
    UINT16               RAM8bitImage[RAM8BIT_SIZE];
    StateChunk           RAM16bit;
    RAMState             RAM16bitState;
    UINT16               RAM16bitImage[RAM16BIT_SIZE];
    StateChunk           GRAM;
    RAMState             GRAMState;
    UINT16               GRAMImage[GRAM_SIZE];
    StateChunk           ivoice;
    IntellivoiceState    ivoiceState;
    StateChunk           ecs;
    ECSState             ecsState;
    StateChunk           eof;
} IntellivisionState; )

class Intellivision : public Emulator
{
    public:
        Intellivision();

        void SaveState();
        BOOL LoadState();

        BOOL isStateValid(const IntellivisionState* state);

        BOOL SaveState(IntellivisionState* outState);
        BOOL LoadState(const IntellivisionState *inState);

        BOOL SaveStateBuffer(void* outBuffer, size_t bufferSize);
        BOOL LoadStateBuffer(const void* inBuffer, size_t bufferSize);

        BOOL SaveStateFile(const CHAR* filename);
        BOOL LoadStateFile(const CHAR* filename);

        inline size_t StateSize() { return sizeof(IntellivisionState); }

    private:
        //core processors
        CP1610            cpu;
        AY38900           stic;
        AY38914           psg;
    
        //core memories
        RAM         RAM8bit;
        RAM         RAM16bit;
        ROM         execROM;
        ROM         grom;
        GRAM        gram;

        //hand controllers
        HandController    player1Controller;
        HandController    player2Controller;

        //the ECS peripheral
        ECS               ecs;
    
        //the Intellivoice peripheral
        Intellivoice      intellivoice;

        //the stored state of the entire emulator
        IntellivisionState state;
};

#endif
