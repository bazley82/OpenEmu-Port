/*
 Copyright (c) 2014, OpenEmu Team
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
     * Neither the name of the OpenEmu Team nor the
       names of its contributors may be used to endorse or promote products
       derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctime>
#include <sys/time.h>
#include "OSystem.hxx"

OSystem::OSystem()
{
    myNVRamDir = ".";
    mySettings = 0;
    myFrameBuffer = new FrameBuffer();
    vcsSound = new SoundSDL(this);
    mySound = vcsSound;
    mySerialPort = new SerialPort();
    myEventHandler = new EventHandler(this);
    myPropSet = new PropertiesSet(this);
    Paddles::setDigitalSensitivity(5);
    Paddles::setMouseSensitivity(5);
}

OSystem::~OSystem()
{
    delete myFrameBuffer;
    delete mySound;
    delete mySerialPort;
    delete myEventHandler;
    delete myPropSet;
}

void OSystem::logMessage(const string& message, uInt8 level)
{
    //logMsg("%s", message.c_str());
}

bool OSystem::create() { return 1; }

void OSystem::mainLoop() { }

void OSystem::pollEvent() { }

bool OSystem::queryVideoHardware() { return 1; }

void OSystem::stateChanged(EventHandler::State state) { }

void OSystem::setDefaultJoymap(Event::Type event, EventMode mode) { }

void OSystem::setFramerate(float framerate) { }
//void OSystem::setFramerate(float framerate)
//{
//    if(framerate > 0.0)
//    {
//        myDisplayFrameRate = framerate;
//        myTimePerFrame = (uInt32)(1000000.0 / myDisplayFrameRate);
//    }
//}

uInt64 OSystem::getTicks() const
{
    // Gettimeofday natively refers to the UNIX epoch (a set time in the past)
    timeval now;
    gettimeofday(&now, 0);
    
    return uInt64(now.tv_sec) * 1000000 + now.tv_usec;
}

EventHandler::EventHandler(OSystem*)
{
    
}

EventHandler::~EventHandler()
{
    
}

FrameBuffer::FrameBuffer() { }

FrameBuffer::~FrameBuffer() { }

FBInitStatus FrameBuffer::initialize(const string& title, uInt32 width, uInt32 height)
{
    //logMsg("called FrameBuffer::initialize, %d,%d", width, height);
    return kSuccess;
}

void FrameBuffer::refresh() { }

void FrameBuffer::showFrameStats(bool enable) { }

// 0 to <counts> - 1, i_s caches the value of counts
//#define iterateTimes(counts, i) for(unsigned int i = 0, i ## _s = counts; i < (i ## _s); i++)
//void FrameBuffer::setTIAPalette(const uInt32* palette)
//{
//}
