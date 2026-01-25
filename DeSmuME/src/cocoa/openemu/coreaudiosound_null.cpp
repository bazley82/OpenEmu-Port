//
//  coreaudiosound_null.cpp
//  DeSmuME (OpenEmu Plug-in)
//
//  Created by Stuart Carnie on 10/30/19.
//  Copyright © 2019 DeSmuME Team. All rights reserved.
//

#include "coreaudiosound.h"

#include <CoreAudio/CoreAudio.h>


CoreAudioInput::CoreAudioInput()
{
    
}

CoreAudioInput::~CoreAudioInput()
{
    
}

OSStatus CoreAudioInput::InitInputAUHAL(UInt32 deviceID)
{
    return noErr;
    
}

void CoreAudioInput::Start()
{
    
}

void CoreAudioInput::Stop()
{
    
}

size_t CoreAudioInput::Pull()
{
    return 0;
}

bool CoreAudioInput::IsHardwareEnabled() const
{
    return false;
}

bool CoreAudioInput::IsHardwareLocked() const
{
    return false;
}

bool CoreAudioInput::GetPauseState() const
{
    return true;
}

void CoreAudioInput::SetPauseState(bool pauseState)
{
    
}

float CoreAudioInput::GetGain() const
{
    return 0;
}

void CoreAudioInput::SetGain(float normalizedGain)
{
    
}

void CoreAudioInput::UpdateHardwareGain(float normalizedGain)
{
    
}

void CoreAudioInput::UpdateHardwareLock()
{
    
}

void CoreAudioInput::SetCallbackHardwareStateChanged(CoreAudioInputHardwareStateChangedCallback callbackFunc, void *inParam1, void *inParam2)
{
    
}

void CoreAudioInput::SetCallbackHardwareGainChanged(CoreAudioInputHardwareGainChangedCallback callbackFunc, void *inParam1, void *inParam2)
{
    
}



#pragma mark -

CoreAudioOutput::CoreAudioOutput(size_t bufferSamples, size_t sampleSize)
{
    
}

CoreAudioOutput::~CoreAudioOutput()
{
    
}

void CoreAudioOutput::start()
{
    
}

void CoreAudioOutput::pause()
{
    
}

void CoreAudioOutput::unpause()
{
    
}

void CoreAudioOutput::stop()
{
    
}

void CoreAudioOutput::writeToBuffer(const void *buffer, size_t numberSampleFrames)
{
    
}

void CoreAudioOutput::clearBuffer()
{
    
}

void CoreAudioOutput::mute()
{
    
}

void CoreAudioOutput::unmute()
{
    
}

size_t CoreAudioOutput::getAvailableSamples() const
{
    return 0;
}

float CoreAudioOutput::getVolume() const
{
    return 0;
}

void CoreAudioOutput::setVolume(float vol)
{
    
}
