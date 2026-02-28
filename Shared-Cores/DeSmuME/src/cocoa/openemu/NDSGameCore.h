/*
	Copyright (C) 2012-2015 DeSmuME team

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

#import <Cocoa/Cocoa.h>
#import <OpenEmuBase/OEGameCore.h>
#import "OENDSSystemResponderClient.h"
#import "../cocoa_input.h"
#include <libkern/OSAtomic.h>
#include <pthread.h>

@class CocoaDSCheatManager;
@class CocoaDSController;
@class CocoaDSGPU;
@class CocoaDSFirmware;


@interface NDSGameCore : OEGameCore
{
	NSPoint touchLocation;
	NSMutableDictionary *addedCheatsDict;
	NSMutableArray <NSMutableDictionary <NSString *, id> *> *_availableDisplayModes;
	
	CocoaDSCheatManager *cdsCheats;
	CocoaDSController *cdsController;
	CocoaDSGPU *cdsGPU;
	CocoaDSFirmware *cdsFirmware;
	
	NSMutableDictionary <NSString *, NSString *> *_currentDisplayModeInfo;
	OEIntPoint topScreenPosition;
	OEIntPoint btmScreenPosition;
	int displayRotation;
	OEIntRect displayRect;
	
	NSInteger inputID[OENDSButtonCount]; // Key = OpenEmu's input ID, Value = DeSmuME's input ID
	
	uint16_t *displayBuffer;
	BOOL ownsBuffer;
	
	OSSpinLock spinlockDisplayMode;
	pthread_rwlock_t rwlockCoreExecute;
}

@property (strong) CocoaDSCheatManager *cdsCheats;
@property (strong) CocoaDSController *cdsController;
@property (strong) CocoaDSGPU *cdsGPU;
@property (strong) CocoaDSFirmware *cdsFirmware;

@end
