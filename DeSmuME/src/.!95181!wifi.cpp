/*
	Copyright (C) 2007 Tim Seidel
	Copyright (C) 2008-2015 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "types.h"

#ifdef HOST_WINDOWS
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define socket_t    SOCKET 	 
	#define sockaddr_t  SOCKADDR
	#include "windriver.h"
	#define PCAP_DEVICE_NAME description
#else
	#include <unistd.h>
	#include <stdlib.h> 	 
	#include <string.h> 	 
	#include <arpa/inet.h> 	 
	#include <sys/socket.h> 	 
	#define socket_t    int 	 
	#define sockaddr_t  struct sockaddr
	#define closesocket close
	#define PCAP_DEVICE_NAME name
#endif

#include "wifi.h"

#include <assert.h>

#include "armcpu.h"
#include "NDSSystem.h"
#include "debug.h"
#include "bits.h"
#include "registers.h"

#ifndef INVALID_SOCKET 	 
	#define INVALID_SOCKET  (socket_t)-1 	 
#endif 

#define BASEPORT 7000

const u8 BroadcastMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool bWFCUserWarned = false;

#ifdef EXPERIMENTAL_WIFI_COMM
socket_t wifi_socket = INVALID_SOCKET;
sockaddr_t sendAddr;
#ifndef WIN32
#include "pcap/pcap.h"
#endif
pcap_t *wifi_bridge = NULL;
#endif

//sometimes this isnt defined
#ifndef PCAP_OPENFLAG_PROMISCUOUS
#define PCAP_OPENFLAG_PROMISCUOUS 1
#endif

static WifiHandler _defaultHandler;
WifiHandler *CurrentWifiHandler = &_defaultHandler;

wifimac_t wifiMac;
SoftAP_t SoftAP;
int wifi_lastmode;

/*******************************************************************************

	WIFI TODO

	- emulate transmission delays for Beacon and Extra transfers
	- emulate delays when receiving as well (may need some queuing system)
	- take transfer rate and preamble into account
	- figure out RFSTATUS and RFPINS

 *******************************************************************************/

/*******************************************************************************

	Firmware info needed for if no firmware image is available
	see: http://www.akkit.org/info/dswifi.htm#WifiInit

	written in bytes, to avoid endianess issues

 *******************************************************************************/

u8 FW_Mac[6] 			= { 0x00, 0x09, 0xBF, 0x12, 0x34, 0x56 };

const u8 FW_WIFIInit[32] 		= { 0x02,0x00,  0x17,0x00,  0x26,0x00,  0x18,0x18,
							0x48,0x00,  0x40,0x48,  0x58,0x00,  0x42,0x00,
							0x40,0x01,  0x64,0x80,  0xE0,0xE0,  0x43,0x24,
							0x0E,0x00,  0x32,0x00,  0xF4,0x01,  0x01,0x01 };
const u8 FW_BBInit[105] 		= { 0x6D, 0x9E, 0x40, 0x05,
							0x1B, 0x6C, 0x48, 0x80,
							0x38, 0x00, 0x35, 0x07,
							0x00, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00,
							0xb0, 0x00, 0x04, 0x01,
							0xd8, 0xff, 0xff, 0xc7,
							0xbb, 0x01, 0xb6, 0x7f,
							0x5a, 0x01, 0x3f, 0x01,
							0x3f, 0x36, 0x36, 0x00,
							0x78, 0x28, 0x55, 0x08,
							0x28, 0x16, 0x00, 0x01,
							0x0e, 0x20, 0x02, 0x98,
							0x98, 0x1f, 0x0a, 0x08,
							0x04, 0x01, 0x00, 0x00,
							0x00, 0xff, 0xff, 0xfe,
							0xfe, 0xfe, 0xfe, 0xfc,
							0xfa, 0xfa, 0xf8, 0xf8,
							0xf6, 0xa5, 0x12, 0x14,
							0x12, 0x41, 0x23, 0x03,
							0x04, 0x70, 0x35, 0x0E,
							0x16, 0x16, 0x00, 0x00,
							0x06, 0x01, 0xff, 0xfe,
							0xff, 0xff, 0x00, 0x0e,
							0x13, 0x00, 0x00, 0x28,
							0x1c
						  };
const u8 FW_RFInit[36] 		= { 0x07, 0xC0, 0x00,
							0x03, 0x9C, 0x12,
							0x28, 0x17, 0x14,
							0xba, 0xe8, 0x1a,
							0x6f, 0x45, 0x1d,
							0xfa, 0xff, 0x23,
							0x30, 0x1d, 0x24,
							0x01, 0x00, 0x28,
							0x00, 0x00, 0x2c,
							0x03, 0x9c, 0x06,
							0x22, 0x00, 0x08,
							0x6f, 0xff, 0x0d
						  };
const u8 FW_RFChannel[6*14]	= { 0x28, 0x17, 0x14,		/* Channel 1 */
							0xba, 0xe8, 0x1a,		
							0x37, 0x17, 0x14,		/* Channel 2 */
							0x46, 0x17, 0x19,		
							0x45, 0x17, 0x14,		/* Channel 3 */
							0xd1, 0x45, 0x1b,		
							0x54, 0x17, 0x14,		/* Channel 4 */
							0x5d, 0x74, 0x19,		
							0x62, 0x17, 0x14,		/* Channel 5 */
							0xe9, 0xa2, 0x1b,		
							0x71, 0x17, 0x14,		/* Channel 6 */
							0x74, 0xd1, 0x19,		
							0x80, 0x17, 0x14,		/* Channel 7 */
							0x00, 0x00, 0x18,
							0x8e, 0x17, 0x14,		/* Channel 8 */
							0x8c, 0x2e, 0x1a,
							0x9d, 0x17, 0x14,		/* Channel 9 */
							0x17, 0x5d, 0x18,
							0xab, 0x17, 0x14,		/* Channel 10 */
							0xa3, 0x8b, 0x1a,
							0xba, 0x17, 0x14,		/* Channel 11 */
							0x2f, 0xba, 0x18,
							0xc8, 0x17, 0x14,		/* Channel 12 */
							0xba, 0xe8, 0x1a,
							0xd7, 0x17, 0x14,		/* Channel 13 */
							0x46, 0x17, 0x19,
							0xfa, 0x17, 0x14,		/* Channel 14 */
							0x2f, 0xba, 0x18
						  };
const u8 FW_BBChannel[14]		= { 0xb3, 0xb3, 0xb3, 0xb3, 0xb3,	/* channel  1- 6 */
							0xb4, 0xb4, 0xb4, 0xb4, 0xb4,	/* channel  7-10 */ 
							0xb5, 0xb5,						/* channel 11-12 */
							0xb6, 0xb6						/* channel 13-14 */
						  };

FW_WFCProfile FW_WFCProfile1 = {"SoftAP",
								"",
								"",
								"",
								"",
								"",
								{0, 0, 0, 0},
								{0, 0, 0, 0},
								{0, 0, 0, 0},
								{0, 0, 0, 0},
								0,
								"",
								0,
								0,
								0,
								{0, 0, 0, 0, 0, 0, 0},
								0,
								{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{0, 0}
							   };

FW_WFCProfile FW_WFCProfile2 = {"",
								"",
								"",
								"",
								"",
								"",
								{0, 0, 0, 0},
								{0, 0, 0, 0},
								{0, 0, 0, 0},
								{0, 0, 0, 0},
								0,
								"",
								0,
								0,
								0xFF,
								{0, 0, 0, 0, 0, 0, 0},
								0,
								{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{0, 0}
							   };

FW_WFCProfile FW_WFCProfile3 = {"",
								"",
								"",
								"",
								"",
								"",
								{0, 0, 0, 0},
								{0, 0, 0, 0},
								{0, 0, 0, 0},
								{0, 0, 0, 0},
								0,
								"",
								0,
								0,
								0xFF,
								{0, 0, 0, 0, 0, 0, 0},
								0,
								{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{0, 0}
							   };

/*******************************************************************************

	Communication interface

 *******************************************************************************/

struct WifiComInterface
{
	bool (*Init)();
	void (*DeInit)();
	void (*Reset)();
	void (*SendPacket)(u8* packet, u32 len);
	void (*msTrigger)();
};

#ifdef EXPERIMENTAL_WIFI_COMM
bool SoftAP_Init();
void SoftAP_DeInit();
void SoftAP_Reset();
void SoftAP_SendPacket(u8 *packet, u32 len);
void SoftAP_msTrigger();

WifiComInterface CI_SoftAP = {
	SoftAP_Init,
	SoftAP_DeInit,
	SoftAP_Reset,
	SoftAP_SendPacket,
	SoftAP_msTrigger
};

bool Adhoc_Init();
void Adhoc_DeInit();
void Adhoc_Reset();
void Adhoc_SendPacket(u8* packet, u32 len);
void Adhoc_msTrigger();

WifiComInterface CI_Adhoc = {
        Adhoc_Init,
        Adhoc_DeInit,
        Adhoc_Reset,
        Adhoc_SendPacket,
        Adhoc_msTrigger
};
#endif

WifiComInterface* wifiComs[] = {
#ifdef EXPERIMENTAL_WIFI_COMM
	&CI_Adhoc,
	&CI_SoftAP,
#endif
	NULL
};
WifiComInterface* wifiCom;

/*******************************************************************************

	Logging

 *******************************************************************************/

// 0: disable logging
// 1: lowest logging, shows most important messages such as errors
// 2: low logging, shows things such as warnings
// 3: medium logging, for debugging, shows lots of stuff
// 4: high logging, for debugging, shows almost everything, may slow down
// 5: highest logging, for debugging, shows everything, may slow down a lot
#define WIFI_LOGGING_LEVEL 3

#define WIFI_LOG_USE_LOGC 0

#if (WIFI_LOGGING_LEVEL >= 1)
	#if WIFI_LOG_USE_LOGC
		#define WIFI_LOG(level, ...) if(level <= WIFI_LOGGING_LEVEL) LOGC(8, "WIFI: " __VA_ARGS__);
	#else
		#define WIFI_LOG(level, ...) if(level <= WIFI_LOGGING_LEVEL) printf("WIFI: " __VA_ARGS__);
	#endif
#else
#define WIFI_LOG(level, ...) {}
#endif

/*******************************************************************************

	Hax

 *******************************************************************************/

// TODO: find the right value
