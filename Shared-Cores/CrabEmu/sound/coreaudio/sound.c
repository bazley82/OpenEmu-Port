/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013 Lawrence Sebald

    CrabEmu is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 
    as published by the Free Software Foundation.

    CrabEmu is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CrabEmu; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <AudioUnit/AudioUnit.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <mach/task.h>
#include <mach/mach_init.h>
#include <mach/semaphore.h>

#include "CrabEmu.h"
#include "sound.h"

#define FRAMES_TO_BUFFER    4
#define FRAME_LOW_HALF      (FRAMES_TO_BUFFER / 2)
#define SAMPLE_RATE         44100

#ifndef __MAC_OS_X_VERSION_10_6
#define __MAC_OS_X_VERSION_10_6 1060
#endif

/* Workarounds for APIs changed in Mac OS X 10.6. */
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_OS_X_VERSION_10_6
typedef AudioComponent Component;
typedef AudioComponentDescription ComponentDescription;

#define FindNextComponent AudioComponentFindNext
#define OpenAComponent AudioComponentInstanceNew
#define CloseComponent AudioComponentInstanceDispose
#endif

#define UNUSED __UNUSED__

static const UInt32 NTSC_SAMPLES_PER_FRAME = 735;  /* SAMPLE_RATE / 60 */
static const UInt32 PAL_SAMPLES_PER_FRAME = 882;   /* SAMPLE_RATE / 50 */

static AudioUnit outputAU;
static unsigned char *buffer;
static UInt32 buffer_size;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static UInt32 read_pos, write_pos;
static int paused = 0;
static semaphore_t sem;
static UInt32 samples_consumed = 0, samples_per_frame;
static int initted = 0;

static OSStatus sound_callback(void *inRefCon UNUSED,
                               AudioUnitRenderActionFlags *ioActionFlags UNUSED,
                               const AudioTimeStamp *inTimeStamp UNUSED,
                               UInt32 inBusNumber UNUSED,
                               UInt32 inNumFrames UNUSED,
                               AudioBufferList *ioData) {
    UInt32 len = ioData->mBuffers[0].mDataByteSize, len2;
    void *ptr = ioData->mBuffers[0].mData;

    pthread_mutex_lock(&mutex);

    if(paused) {
        memset(ptr, 0, len);
    }
    else if((read_pos + len > write_pos && write_pos > read_pos)) {
        memset(ptr, 0, len);
        samples_consumed += len / 2;
    }
    else {
        samples_consumed += len / 2;

        if(read_pos + len > buffer_size) {
            len2 = buffer_size - read_pos;
            len -= len2;

            memcpy(ptr, buffer + read_pos, len2);
            memcpy(ptr + len2, buffer, len);
            read_pos = len;
        }
        else {
            memcpy(ptr, buffer + read_pos, len);
            read_pos += len;

            if(read_pos == buffer_size)
                read_pos = 0;
        }
    }

    /* See if we need to increment our semaphore */
    while(samples_consumed > samples_per_frame) {
        semaphore_signal(sem);
        samples_consumed -= samples_per_frame;
    }

    pthread_mutex_unlock(&mutex);

    return noErr;
}

void sound_update_buffer(signed short *buf, int length) {
    pthread_mutex_lock(&mutex);

    if(write_pos + length > buffer_size) {
        int l1, l2;

        l1 = buffer_size - write_pos;
        l2 = length - l1;

        memcpy(buffer + write_pos, buf, l1);
        memcpy(buffer, ((unsigned char *)buf) + l1, l2);

        write_pos = l2;
    }
    else {
        memcpy(buffer + write_pos, buf, length);
        write_pos += length;
    }

    pthread_mutex_unlock(&mutex);
}

int sound_init(int channels, int region) {
    OSStatus error = noErr;
    ComponentDescription desc;
    AudioStreamBasicDescription basic_desc;
    Component comp;
    AURenderCallbackStruct callback;
    UInt32 bufsz;
    int rv = 0;

    if(initted)
        return 0;

    /* Figure out how big our internal buffer will be */
    if(region == VIDEO_NTSC)
        samples_per_frame = NTSC_SAMPLES_PER_FRAME * channels;
    else
        samples_per_frame = PAL_SAMPLES_PER_FRAME * channels;

    buffer_size = samples_per_frame * FRAMES_TO_BUFFER * 2;

    /* Allocate our buffer */
    buffer = calloc(buffer_size, 1);
    read_pos = 0;
    write_pos = samples_per_frame * FRAME_LOW_HALF * 2;

    /* Find the default audio output unit */
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    comp = FindNextComponent(NULL, &desc);

    if(comp == NULL) {
        rv = -1;
        goto err1;
    }

    /* We got the component, make sure we can actually open it up. */
    error = OpenAComponent(comp, &outputAU);

    if(error != noErr) {
        rv = -2;
        goto err1;
    }

    /* Set up the AudioStreamBasicDescription - 16-bit Stereo PCM @ 44100Hz */
    basic_desc.mFormatID = kAudioFormatLinearPCM;
    basic_desc.mFormatFlags = kLinearPCMFormatFlagIsPacked | 
        kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian;
    basic_desc.mChannelsPerFrame = channels;
    basic_desc.mSampleRate = SAMPLE_RATE;
    basic_desc.mBitsPerChannel = 16;
    basic_desc.mFramesPerPacket = 1;
    basic_desc.mBytesPerFrame = 2 * channels;
    basic_desc.mBytesPerPacket = 2 * channels;

    /* Set the stream format that we set up above */
    error = AudioUnitSetProperty(outputAU, kAudioUnitProperty_StreamFormat,
                                 kAudioUnitScope_Input, 0, &basic_desc,
                                 sizeof(basic_desc));

    if(error != noErr) {
        rv = -3;
        goto err2;
    }

    /* Set the callback for getting sound data. */
    callback.inputProc = &sound_callback;
    callback.inputProcRefCon = NULL;

    error = AudioUnitSetProperty(outputAU, kAudioUnitProperty_SetRenderCallback,
                                 kAudioUnitScope_Input, 0, &callback,
                                 sizeof(callback));

    if(error != noErr) {
        rv = -4;
        goto err2;
    }

    bufsz = 512;
    error = AudioUnitSetProperty(outputAU,
                                 kAudioUnitProperty_MaximumFramesPerSlice,
                                 kAudioUnitScope_Global, 0, &bufsz,
                                 sizeof(UInt32));

    if(error != noErr) {
        rv = -7;
        goto err3;
    }

    /* Initialize the Audio Unit for our use now that its set up. */
    error = AudioUnitInitialize(outputAU);

    if(error != noErr) {
        rv = -8;
        goto err3;
    }

    /* Create the semaphore */
    if(semaphore_create(mach_task_self(), &sem, SYNC_POLICY_FIFO,
                        FRAME_LOW_HALF)) {
        rv = -10;
        goto err4;
    }

    /* Pause the audio output... */
    paused = 1;

    /* We're ready to output now */
    error = AudioOutputUnitStart(outputAU);

    if(error != noErr) {
        rv = -9;
        goto err5;
    }

    initted = 1;

    return 0;

    /* Error conditions. Errors cause cascading deinitialization, so hence this
       chain of labels. */
err5:
    semaphore_destroy(mach_task_self(), sem);

err4:
    AudioUnitUninitialize(outputAU);

err3:
    callback.inputProc = NULL;
    callback.inputProcRefCon = NULL;

    AudioUnitSetProperty(outputAU, kAudioUnitProperty_SetRenderCallback,
                         kAudioUnitScope_Input, 0, &callback,
                         sizeof(callback));

err2:
    CloseComponent(outputAU);

err1:
    return rv;
}

void sound_shutdown(void) {
    AURenderCallbackStruct callback;

    if(!initted)
        return;

    /* Stop the Audio Unit from playing any further */
    AudioOutputUnitStop(outputAU);

    /* Clear the callback */
    callback.inputProc = NULL;
    callback.inputProcRefCon = NULL;

    AudioUnitSetProperty(outputAU, kAudioUnitProperty_SetRenderCallback,
                         kAudioUnitScope_Input, 0, &callback,
                         sizeof(callback));

    /* Uninitialize the Audio Unit, now that we're done with it */
    AudioUnitUninitialize(outputAU);

    /* Close it, we're done */
    CloseComponent(outputAU);

    /* Clean up the semaphore */
    semaphore_destroy(mach_task_self(), sem);

    free(buffer);
    buffer = NULL;

    initted = 0;
}

void sound_reset_buffer(void) {
    if(!initted)
        return;

    read_pos = 0;
    write_pos = samples_per_frame * FRAME_LOW_HALF * 2;
    memset(buffer, 0, buffer_size);
}

void sound_pause() {
    paused = 1;
}

void sound_unpause() {
    paused = 0;
}

void sound_wait(void) {
    if(!initted)
        return;

    semaphore_wait(sem);
}
