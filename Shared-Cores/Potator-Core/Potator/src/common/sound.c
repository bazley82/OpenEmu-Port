////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
#include "sound.h"
#include <math.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


uint8* stream;

static uint8    sound_regs[2][4];
static uint32   sound_channel_size[2];
static uint32	sound_channel_pos[2];

static int16    *sound_buffers[2];
static uint8    audioDma_regs[8];

//unsigned short		BPS;
#define BPS			11025
#define BPSMAX		44100
#define BPSMIN		11025
#define BUFSIZE		BPS
#define POFF		128
#define PDIV		3
#define PH			POFF+PDIV*8
#define PL			POFF-PDIV*7

uint8       noise_buffer[BUFSIZE];
uint8       voice_buffer[BUFSIZE];
uint8       dma_buffer[BUFSIZE];
uint8       final_buffer[BUFSIZE];

BOOL		audio_FmVoicePlaying;
BOOL        audio_NoiseVoicePlaying;
BOOL        audio_AudioDmaVoicePlaying;

BOOL		audio_FmVoicePlayingOld;
BOOL        audio_NoiseVoicePlayingOld;
BOOL        audio_AudioDmaVoicePlayingOld;

//uint8       *RandData;
//uint8       RandData[BUFSIZE];
unsigned int audio_mrand(unsigned int Degree);

const long TblChVol[16]={				// n/15 n=0~15
	-10000,-2352,-1750,-1398,-1148,-954,-796,-662,
    -546,-444,-352,-269,-194,-124,-60,0
};

void audio_NoiseVoiceFreq(int freq, int volume)
{
	int j;
    for(j=0;j<(BUFSIZE);j++)
		noise_buffer[j] = ((audio_mrand(15)*volume)/127);

/*
	memcpy(audio_NoiseWave.lpData,		RandData, audio_NoiseWave.dwLength);
	AWriteAudioData(&audio_NoiseWave,	0, audio_NoiseWave.dwLength);
*/
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_FmVoicePlay(int channel, BOOL bPlay)
{
	if (audio_FmVoicePlaying == bPlay)
		return;

	audio_FmVoicePlaying = bPlay;

	if (bPlay)
		;//APlayVoice(audio_FmVoice[channel], &audio_FmWave[channel]);
	else
		;//AStopVoice(audio_FmVoice[channel]);

}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_FmVoiceFreq(int channel, int freq, int volume)
{
	int size;
	int j;

	volume = (127*volume)/15;

	//fprintf(log_get()," volume: %i\n", volume);
	if (freq==0)
	{
		audio_FmVoicePlay(channel, FALSE);
		return;
	}
	size = (BUFSIZE/freq);
	
	if (size == 0)
	{
		audio_FmVoicePlay(channel, FALSE);
		return;
	}

    for(j=0;j<(BUFSIZE);j++)
	{
		voice_buffer[j] = (((uint32)(((j%size)<(size>>1))?127:-128))*volume)/127;
//		RandData[j] = (char)(127.0f*sin((((float)j)*((float)freq)*2*3.14159)/((float)BUFSIZE)));
	}
/*
	memcpy(audio_FmWave[channel].lpData,		RandData, audio_FmWave[channel].dwLength);
	AWriteAudioData(&audio_FmWave[channel],	0, audio_FmWave[channel].dwLength);
*/
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_AudioDmaSetSample(uint32 offset, uint32 length, uint32 freq, uint32 cartSpace)
{
	float scale = ((float)freq)/((float)length);
	int i;

	for (i = 0; i < length; i++)
	{
		int16 data;

		int offs = ((i>>1)*scale);
		if (offs>= (length>>1))
			data =0;
		else
			data = (cartSpace)?(memorymap_getRomPointer()[offset + offs]):Rd6502(offset + offs);
		
		if (i&0x01)
			data = ((data>>4)-8)*8;
		else
			data = ((data&0x0f)-8)*8;

		dma_buffer[i] = (char)(data);
	}
	//memcpy(audio_AudioDmaWave.lpData, RandData, audio_AudioDmaWave.dwLength);
	//AWriteAudioData(&audio_AudioDmaWave,	0, audio_AudioDmaWave.dwLength);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_NoiseVoiceVolume(int volume)
{
//	ASetVoiceVolume(audio_NoiseVoice, TblChVol[volume]);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_AudioDmaVoiceVolume(int volume)
{
//	ASetVoiceVolume(audio_AudioDmaVoice, TblChVol[volume]);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_NoiseVoicePlay(BOOL bPlay)
{
	if (audio_NoiseVoicePlaying == bPlay)
		return;

	audio_NoiseVoicePlaying = bPlay;

	if (bPlay)
		;//APlayVoice(audio_NoiseVoice, &audio_NoiseWave);
	else
		;//AStopVoice(audio_NoiseVoice);

}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_AudioDmaVoicePlay(BOOL bPlay)
{
	if (audio_AudioDmaVoicePlaying == bPlay)
		return;

	audio_AudioDmaVoicePlaying = bPlay;

	if (bPlay)
		;//APlayVoice(audio_AudioDmaVoice, &audio_AudioDmaWave);
	else
		;//AStopVoice(audio_AudioDmaVoice);

}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_FmVoiceVolume(int channel, int volume)
{
//	ASetVoiceVolume(audio_FmVoice[channel], TblChVol[volume]);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_turnSound(BOOL bOn)
{
/*	if (bOn)
	{
		audio_FmVoicePlay(0, audio_FmVoicePlayingOld[0]);
		audio_FmVoicePlay(1, audio_FmVoicePlayingOld[1]);
		audio_NoiseVoicePlay(audio_NoiseVoicePlayingOld);
		audio_AudioDmaVoicePlay(audio_AudioDmaVoicePlayingOld);
	} 
	else
	{
		audio_FmVoicePlayingOld[0] = audio_FmVoicePlaying[0];
		audio_FmVoicePlayingOld[1] = audio_FmVoicePlaying[1];
		audio_NoiseVoicePlayingOld = audio_NoiseVoicePlaying;
		audio_AudioDmaVoicePlayingOld = audio_AudioDmaVoicePlaying;
		audio_FmVoicePlay(0, FALSE);
		audio_FmVoicePlay(1, FALSE);
		audio_NoiseVoicePlay(FALSE);
		audio_AudioDmaVoicePlay(FALSE);
	}
*/
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
unsigned int audio_mrand(unsigned int Degree)
{

	typedef struct {
		unsigned int N;
    	int InputBit;
    	int Mask;
	} POLYNOMIAL;

	static POLYNOMIAL TblMask[]=
	{
		{ 2,BIT(2) ,BIT(0)|BIT(1)},
		{ 3,BIT(3) ,BIT(0)|BIT(1)},
		{ 4,BIT(4) ,BIT(0)|BIT(1)},
		{ 5,BIT(5) ,BIT(0)|BIT(2)},
		{ 6,BIT(6) ,BIT(0)|BIT(1)},
		{ 7,BIT(7) ,BIT(0)|BIT(1)},
		{ 8,BIT(8) ,BIT(0)|BIT(2)|BIT(3)|BIT(4)},
		{ 9,BIT(9) ,BIT(0)|BIT(4)},
		{10,BIT(10),BIT(0)|BIT(3)},
		{11,BIT(11),BIT(0)|BIT(2)},
		{12,BIT(12),BIT(0)|BIT(1)|BIT(4)|BIT(6)},
		{13,BIT(13),BIT(0)|BIT(1)|BIT(3)|BIT(4)},
		{14,BIT(14),BIT(0)|BIT(1)|BIT(4)|BIT(5)},
		{15,BIT(15),BIT(0)|BIT(1)},
		{0,0,0},
	};

	static POLYNOMIAL *pTbl=TblMask;
	int ShiftReg=TblMask->InputBit-1;
	int XorReg=0;
	int Masked;

    if(pTbl->N!=Degree)
    {
    	pTbl=TblMask;
		while(pTbl->N)
		{
			if(pTbl->N==Degree)
			{
				break;
			}
        	pTbl++;
		}
		if(!pTbl->N)
		{
			pTbl--;
		}

    	ShiftReg&=pTbl->InputBit-1;
		if(!ShiftReg)
		{
			ShiftReg=pTbl->InputBit-1;
		}
    }

	Masked=ShiftReg&pTbl->Mask;
	while(Masked)
	{
		XorReg^=Masked&0x01;
		Masked>>=1;
	}

	if(XorReg)
	{
		ShiftReg|=pTbl->InputBit;
	}
	else
	{
		ShiftReg&=~pTbl->InputBit;
	}
	ShiftReg>>=1;

	return ShiftReg;
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_createFmVoices()
{
	int i;
	for (i = 0;i<2;i++)
	{
		/*ACreateAudioVoice(&audio_FmVoice[i]);
		ASetVoiceVolume  ( audio_FmVoice[i], AUDIO_MAX_VOLUME);
		ASetVoicePanning ( audio_FmVoice[i], AUDIO_MAX_PANNING>>1);
		ASetVoiceFrequency(audio_FmVoice[i],audio_info.nSampleRate);

		audio_FmWave[i].nSampleRate		= audio_info.nSampleRate;
		audio_FmWave[i].dwLength		= BUFSIZE;
		audio_FmWave[i].dwLoopStart		= 0;
		audio_FmWave[i].dwLoopEnd		= audio_FmWave[i].dwLength;
		audio_FmWave[i].wFormat			= AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		ACreateAudioData(&audio_FmWave[i]);*/
	}
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_createNoiseVoice()
{
/*	ACreateAudioVoice(&audio_NoiseVoice);
	ASetVoiceVolume  ( audio_NoiseVoice, AUDIO_MAX_VOLUME);
	ASetVoicePanning ( audio_NoiseVoice, AUDIO_MAX_PANNING>>1);
	ASetVoiceFrequency(audio_NoiseVoice,audio_info.nSampleRate);

	audio_NoiseWave.nSampleRate		= audio_info.nSampleRate;
	audio_NoiseWave.dwLength		= BUFSIZE;
	audio_NoiseWave.dwLoopStart		= 0;
	audio_NoiseWave.dwLoopEnd		= audio_NoiseWave.dwLength;
	audio_NoiseWave.wFormat			= AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
	ACreateAudioData(&audio_NoiseWave);
*/
	audio_NoiseVoiceFreq(50, 127);

}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_createAudioDmaVoice()
{
/*	ACreateAudioVoice(&audio_AudioDmaVoice);
	ASetVoiceVolume  ( audio_AudioDmaVoice, AUDIO_MAX_VOLUME);
	ASetVoicePanning ( audio_AudioDmaVoice, AUDIO_MAX_PANNING>>1);
	ASetVoiceFrequency(audio_AudioDmaVoice, audio_info.nSampleRate);

	audio_AudioDmaWave.nSampleRate		= audio_info.nSampleRate;
	audio_AudioDmaWave.dwLength		= BUFSIZE;
	audio_AudioDmaWave.dwLoopStart		= 0;
	audio_AudioDmaWave.dwLoopEnd		= audio_AudioDmaWave.dwLength;
	audio_AudioDmaWave.wFormat			= AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;// | AUDIO_FORMAT_LOOP;
	ACreateAudioData(&audio_AudioDmaWave);
*/
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_done()
{/*
	fprintf(log_get(), "sound: fm voice %i was %s\n", 0, (audio_FmVoicePlaying[0]?"playing":"not playing"));
	fprintf(log_get(), "sound: fm voice %i was %s\n", 1, (audio_FmVoicePlaying[1]?"playing":"not playing"));
	fprintf(log_get(), "sound: noise voice was %s\n", (audio_NoiseVoicePlaying?"playing":"not playing"));
	fprintf(log_get(), "sound: audio dma voice was %s\n", (audio_AudioDmaVoicePlaying?"playing":"not playing"));
*/
/*	for (int i = 0;i<2;i++)
	{
		ADestroyAudioData(&audio_FmWave[i]);
		ADestroyAudioVoice(audio_FmVoice[i]);
	}
	ADestroyAudioData(&audio_NoiseWave);
	ADestroyAudioVoice(audio_NoiseVoice);
	ADestroyAudioData(&audio_AudioDmaWave);
	ADestroyAudioVoice(audio_AudioDmaVoice);
	ACloseVoices();
    ACloseAudio();
*/
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void audio_init()
{
/*    UINT		rc;
	UINT		nDevId;

	fprintf(log_get(),"audio: using seal audio library\n");
    // initialize audio library
	AInitialize();

    // show registered device drivers
    fprintf(log_get(),"audio: registered sound devices:\n");
    for (nDevId = 0; nDevId < AGetAudioNumDevs(); nDevId++) {
        AGetAudioDevCaps(nDevId, &audio_caps);
        fprintf(log_get(),"audio:   %2d. %s\n", nDevId, audio_caps.szProductName);
    }
    // open audio device
    audio_info.nDeviceId = AUDIO_DEVICE_MAPPER;
    audio_info.wFormat   = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;//|AUDIO_MIXER_BASS;
    audio_info.nSampleRate = 11025;
    if ((rc = AOpenAudio(&audio_info)) != AUDIO_ERROR_NONE) 
	{
        CHAR szText[80];
        AGetErrorText(rc, szText, sizeof(szText) - 1);
        fprintf(log_get(),"audio: error: %s\n", szText);
        return;
    }
	
	AOpenVoices(4);*/
	audio_createFmVoices();
	audio_createNoiseVoice();
	audio_createAudioDmaVoice();

	//BPS = currentConfig.SoundRate;
	//RandData = (u8*)malloc(BPS);

}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void sound_init()
{
	//fprintf(log_get(), "sound: init\n");
	//memory_malloc_secure((void**)&sound_buffers[0],  BPS*sizeof(int16), "Right sound buffer");
	//memory_malloc_secure((void**)&sound_buffers[1],  BPS*sizeof(int16), "Left sound buffer");
	
	//memset(sound_buffers[0], 0x00, BPS*sizeof(int16));
	//memset(sound_buffers[1], 0x00, BPS*sizeof(int16));

	//iprintf("audio: using seal audio library\n");

	audio_init();
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void sound_reset()
{
//	fprintf(log_get(), "sound: reset\n");
	memset(sound_regs, 0, 8);
	memset(sound_channel_size, 0, 2*sizeof(uint32));
	memset(sound_channel_pos,  0, 2*sizeof(uint32));
	memset(audioDma_regs, 0, 8);
	audio_FmVoicePlaying = TRUE;
	audio_FmVoicePlay(0, FALSE);
	audio_FmVoicePlay(1, FALSE);
	audio_NoiseVoicePlaying = TRUE;
	audio_NoiseVoicePlay(FALSE);
	audio_AudioDmaVoicePlaying = TRUE;
	audio_AudioDmaVoicePlay(FALSE);

}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void sound_done()
{
//	fprintf(log_get(), "sound: done\n");
	audio_done();
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void sound_audio_dma(uint32 Addr, uint8 data)
{
	// fprintf(log_get(), "sound: writing 0x%2x at 0x%.4x (audio dma voice)\n", data, Addr);
	
	audioDma_regs[Addr&0x07] = data;

	switch (Addr&0x07)
	{

	case 0x04: {
					if (data&0x80)
					{
						uint32 offset = ((uint16)audioDma_regs[0])|(((uint16)audioDma_regs[1])<<8);
						uint32 length = ((uint16)audioDma_regs[2])*32;
						audio_AudioDmaSetSample(offset, length, 12000/(1+(audioDma_regs[3]&0x03)), audioDma_regs[3]&(1<<5));
						if (Rd6502(0x2026)&0x04)
						{
							Wr6502(0x2027, Rd6502(0x2027)|0x02);
							interrupts_irq();
						}
						audio_AudioDmaVoicePlay(FALSE);
						audio_AudioDmaVoicePlay(TRUE);
					}
					else
						audio_AudioDmaVoicePlay(FALSE);
					break;	
			   }
	}
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void sound_noise_write(uint32 Addr, uint8 data)
{
	// fprintf(log_get(), "sound: writing 0x%2x at 0x%.4x (noise voice)\n", data, Addr);

	switch (Addr)
	{
	case 0:
			if ((data&0x07)==0)
				audio_NoiseVoicePlay(FALSE);
			else
			{
				audio_NoiseVoiceFreq((BPS/16)*(15-((data>>4)&0x0f)), (127*(data&0x0f))/15);
				audio_NoiseVoicePlay(TRUE);
			}
			break;
	case 2: if ((data&0x10)||((data&0x0c)!=0))
				audio_NoiseVoicePlay(TRUE);
			else
				audio_NoiseVoicePlay(FALSE);
	}

}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void sound_write(uint32 Addr, uint8 data)
{
	uint32 channel = ((Addr&0x4)>>2);
	Addr&=0x03;
	sound_regs[channel][Addr] = data;

	 // fprintf(log_get(), "sound: writing 0x%.2x at 0x%.4x (channel %i)\n", data, Addr, channel);
	switch (Addr) 
	{
    case 0:
			// fprintf(log_get(),"sound: channel %i, freq 0x%.2x\n", channel, data);
    case 1:
	case 2: 
			if (sound_regs[channel][0]) 
			{
				uint32 v1 = sound_regs[channel][0];
				uint32 v2 = sound_regs[channel][1]&0x07;
				
				uint32 counter = (v1|(v2<<8));
				
				audio_FmVoiceFreq(channel, 4000000/(counter*32), sound_regs[channel][2]&0x0f);
				audio_FmVoicePlay(channel, TRUE);
			}
			else 
				audio_FmVoicePlay(channel, FALSE);
				
			sound_channel_pos[channel] = 0;
			break;
    }

}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void sound_exec(uint32 cycles)
{
}
