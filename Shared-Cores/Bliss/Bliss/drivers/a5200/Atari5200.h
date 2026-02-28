
#ifndef ATARI_5200_H
#define ATARI_5200_H

#include "core/Emulator.h"
#include "core/cpu/SignalLine.h"
#include "core/cpu/6502c.h"
#include "core/memory/RAM.h"
#include "core/memory/ROM.h"
#include "core/video/Antic.h"
#include "core/video/GTIA.h"
#include "core/audio/Pokey.h"
#include "JoyPad.h"

class Atari5200 : public Emulator
{

    public:
        Atari5200();
#if 0
        void SaveState();
        BOOL LoadState();

        BOOL isStateValid(const Atari5200State* state);

        BOOL SaveState(Atari5200State* outState);
        BOOL LoadState(const Atari5200State *inState);
#endif
        BOOL SaveStateBuffer(void* outBuffer, size_t bufferSize);
        BOOL LoadStateBuffer(const void* inBuffer, size_t bufferSize);

        BOOL SaveStateFile(const CHAR* filename);
        BOOL LoadStateFile(const CHAR* filename);

        inline size_t StateSize() { return 0; }

    private:
        JoyPad      leftInput;
        JoyPad      rightInput;
        _6502c      cpu;
        Antic       antic;
        GTIA        gtia;
        Pokey       pokey;

        ROM         biosROM;
        RAM         ram;

};

#endif
