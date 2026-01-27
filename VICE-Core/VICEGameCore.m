/*
 Copyright (c) 2023, OpenEmu Team

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

#import "VICEGameCore.h"
#import <OpenEmuBase/OERingBuffer.h>
#import <OpenGL/gl.h>
#include <pthread.h>
#include <stdbool.h>

#include "main.h"
#include "video.h"
#include "video-canvas.h"
#include "videoarch.h"
#include "machine.h"
#include "archdep.h"
#include "gfxoutput.h"
#include "init.h"
#include "resources.h"
#include "joystick.h"
#include "keyboard.h"
#include "vsyncapi.h"
#include "ui.h"
#include "palette.h"
#include "lib.h"

// OpenEmu Input Constants for C64 (matches System Plugin)
// Usually Port 2 is default for games on C64
#define C64_JOYSTICK_PORT 2

/* 
 * VICE Joystick Bits:
 * UP    = 0x01
 * DOWN  = 0x02
 * LEFT  = 0x04
 * RIGHT = 0x08
 * FIRE  = 0x10
 */


// Forward declare logic
extern int main_program(int argc, char **argv);

// Global or static constraints for this singleton core
// Global or static constraints for this singleton core
// Global or static constraints for this singleton core
static struct video_canvas_s *curr_canvas = NULL;
@class VICEGameCore;
static VICEGameCore *g_core = nil;
static volatile bool g_quitEmulator = false;

@interface VICEGameCore ()
@property (nonatomic, assign) uint32_t *videoBuffer;
@property (nonatomic, assign) BOOL frameFinished;
@property (nonatomic, assign) BOOL emulatorRunning;
+ (id)current;
@end

// Stub Controller to force Principal Class resolution within the bundle
@interface VICEGameCoreController : OEGameCoreController
@end
@implementation VICEGameCoreController
+ (void)initialize {
    NSLog(@"[VICE] VICEGameCoreController initialized");
}
- (id)initWithBundle:(NSBundle *)bundle {
    self = [super initWithBundle:bundle];
    FILE *f = fopen("/tmp/vice_controller_init.txt", "w");
    if (f) {
        fprintf(f, "VICEGameCoreController initWithBundle called at %ld\n", time(NULL));
        fclose(f);
    }
    return self;
}

- (id)newGameCore {
    FILE *f = fopen("/tmp/vice_newgamecore.txt", "w");
    if (f) {
        fprintf(f, "VICEGameCoreController newGameCore called at %ld\n", time(NULL));
        fclose(f);
    }
    return [super newGameCore];
}
@end

static void log_debug_marker(const char *msg) {
    FILE *f = fopen("/Users/barriesanders/Documents/vice_debug.log", "a");
    if (f) {
        fprintf(f, "[%lld] %s\n", (long long)time(NULL), msg);
        fclose(f);
    }
}

@implementation VICEGameCore (Debug)
+ (void)initialize {
    log_debug_marker("VICEGameCore class initialized (Runtime loaded)");
}
@end

static int videoWidth = 384;
static int videoHeight = 272;

// Archdep Stubs and Overrides
// Archdep Overrides
// Removed duplicates provided by shared/headless archdep
char *archdep_default_global_resource_file_name(void) { return NULL; }

char *archdep_get_vice_datadir(void) {
    static char dataDirPath[1024];
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        NSBundle *bundle = [NSBundle bundleForClass:[VICEGameCore class]];
        NSString *path = [bundle pathForResource:@"data" ofType:nil];
        if (path) {
            NSLog(@"[VICE] Data dir found at: %{public}@", path);
            strncpy(dataDirPath, [path fileSystemRepresentation], sizeof(dataDirPath) - 1);
            
            // Verify kernal and basic
            NSString *c64kernal = [path stringByAppendingPathComponent:@"C64/kernal"];
            NSString *c64basic = [path stringByAppendingPathComponent:@"C64/basic"];
            NSString *c64chargen = [path stringByAppendingPathComponent:@"C64/chargen"];
            
            if ([[NSFileManager defaultManager] fileExistsAtPath:c64kernal]) {
                 NSLog(@"[VICE] C64 Kernal found.");
            } else {
                 NSLog(@"[VICE] ERROR: C64 Kernal NOT found at %{public}@", c64kernal);
            }
            if ([[NSFileManager defaultManager] fileExistsAtPath:c64basic]) {
                 NSLog(@"[VICE] C64 Basic found.");
            } else {
                 NSLog(@"[VICE] ERROR: C64 Basic NOT found at %{public}@", c64basic);
            }
            if ([[NSFileManager defaultManager] fileExistsAtPath:c64chargen]) {
                 NSLog(@"[VICE] C64 Chargen found.");
            } else {
                 NSLog(@"[VICE] ERROR: C64 Chargen NOT found at %{public}@", c64chargen);
            }
        } else {
            NSLog(@"[VICE] Error: Could not locate 'data' directory in bundle!");
            dataDirPath[0] = '\0';
        }
    });
    return lib_strdup(dataDirPath);
}

// Redirect VICE logging to file
int archdep_default_logger(const char *prefix, const char *log_text) {
    // Determine log path once
    static NSString *logPath = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *docsDir = [paths firstObject];
        logPath = [docsDir stringByAppendingPathComponent:@"OpenEmu-VICE-Debug.log"];
        // Reset file
        [[NSFileManager defaultManager] createFileAtPath:logPath contents:nil attributes:nil];
        NSLog(@"[VICE] Logging to: %{public}@", logPath);
        
    });

    if (!logPath) return 0;
    
    // Also log to NSLog for debugging in Console.app
    NSLog(@"[VICE-INT] %s%s", prefix, log_text);
    
    // Append to file
    NSFileHandle *handle = [NSFileHandle fileHandleForWritingAtPath:logPath];
    [handle seekToEndOfFile];
    NSString *line = [NSString stringWithFormat:@"%s%s\n", prefix, log_text];
    [handle writeData:[line dataUsingEncoding:NSUTF8StringEncoding]];
    [handle closeFile];
    
    return 0;
}

int archdep_default_logger_is_terminal(void) {
    return 1;
}

void video_arch_canvas_init(struct video_canvas_s *canvas) {
    NSLog(@"[VICE] video_arch_canvas_init called");
    curr_canvas = canvas;
    canvas->created = 1; // Mark as created so VICE uses it
}

// Stubs from headless/video.c
int video_arch_get_active_chip(void) { return 0; } // VIDEO_CHIP_VICII
int video_arch_cmdline_options_init(void) { return 0; }
int video_arch_resources_init(void) { return 0; }
void video_arch_resources_shutdown(void) {}
char video_canvas_can_resize(struct video_canvas_s *canvas) { return 0; }
struct video_canvas_s *video_canvas_create(struct video_canvas_s *canvas, unsigned int *width, unsigned int *height, int mapped) {
    NSLog(@"[VICE] video_canvas_create called");
    canvas->created = 1;
    return canvas;
}

void video_canvas_destroy(struct video_canvas_s *canvas) {}
void video_canvas_refresh(struct video_canvas_s *canvas, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h) {
    if (!canvas || !canvas->draw_buffer || !canvas->palette) return;

    VICEGameCore *core = (VICEGameCore *)[VICEGameCore current];
    if (!core || !core.videoBuffer) return;

    uint32_t *dest = core.videoBuffer;
    uint8_t *src = canvas->draw_buffer->draw_buffer;
    unsigned int src_pitch = canvas->draw_buffer->draw_buffer_width;
    unsigned int dest_width = videoWidth; 

    palette_entry_t *palette = canvas->palette->entries;

    // Boundary check
    if (yi + h > videoHeight) h = videoHeight - yi;
    if (xi + w > videoWidth) w = videoWidth - xi;

    for (unsigned int y = yi; y < yi + h; y++) {
        uint32_t *line_dest = &dest[y * dest_width];
        uint8_t *line_src = &src[y * src_pitch];
        for (unsigned int x = xi; x < xi + w; x++) {
            palette_entry_t *entry = &palette[line_src[x]];
            // 0xAARRGGBB for GL_BGRA + GL_UNSIGNED_INT_8_8_8_8_REV on Mac
            line_dest[x] = (0xFF << 24) | (entry->red << 16) | (entry->green << 8) | entry->blue;
        }
    }
    
    static int frame_count = 0;
    if (++frame_count % 60 == 0) {
        // Sample center pixel to verify video output
        int center_idx = (videoHeight / 2) * videoWidth + (videoWidth / 2);
        uint32_t sample_pixel = dest[center_idx];
        
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *docsDir = [paths firstObject];
        NSString *logPath = [docsDir stringByAppendingPathComponent:@"vice_executed.log"];
        FILE *f = fopen([logPath UTF8String], "a");
        if (f) {
             fprintf(f, "Frame %d: Center Pixel: 0x%08X\n", frame_count, sample_pixel);
             fclose(f);
        }
    }
}
void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas) {}
int video_canvas_set_palette(struct video_canvas_s *canvas, struct palette_s *palette) {
    canvas->palette = palette;
    return 0;
}
int video_init(void) { return 0; }
void video_shutdown(void) {}

// Duplicate removed
// void video_arch_canvas_shutdown(struct video_canvas_s *canvas) {} - removed earlier
// archdep_thread_init kept unless duplicate
void archdep_free_resources(void) {}
void archdep_init_resources(void) {}
// int archdep_init(int *argc, char **argv) { return 0; } // Removed duplicate
// archdep_shutdown removed (duplicate)


// Joystick Stubs
void joystick_arch_init(void) {}
void joystick_arch_shutdown(void) {}

// UI Stubs - Removed duplicates covered by UI/Statusbar
void ui_set_aspect_mode(int mode) {}
void ui_set_aspect_ratio(float ratio) {}
void ui_set_flipx(int flip) {}
void ui_set_flipy(int flip) {}
void ui_set_glfilter(int filter) {}
void ui_set_rotate(int angle) {}
void ui_set_vsync(int vsync) {}

// VSync Arch - Critical for timing
// The "Sync is ... ms behind" errors suggest we are not sleeping or pacing the emulator.
unsigned long vsyncarch_frequency(void) {
    return 1000000; // 1MHz timer (microseconds)
}

unsigned long vsyncarch_gettime(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)(tv.tv_sec * 1000000 + tv.tv_usec);
}

void vsyncarch_presync(void) {
    // This is called before vsync logic.
    if (g_quitEmulator) {
        main_exit(); // Handle pending quit
    }
    
    // Poll input
    ui_update_lightpen();
    joystick();

    // Ensure we yield to prevent 100% CPU usage if audio is blocking.
    usleep(1000); 
}

void vsyncarch_postsync(void) {
    // Called after vsync logic
    VICEGameCore *core = [VICEGameCore current];
    if (core) {
        [core setFrameFinished:YES];
    }
}

void vsyncarch_init(void) {} // Initialized in main
void vsyncarch_shutdown(void) {} // Shutdown in main

// UI Action Map Stubs (missing if uiactions.o is excluded)
void *ui_action_map_get(void) { return NULL; }
void *ui_action_map_get_by_hotkey(int key) { return NULL; }
void ui_action_map_set_hotkey(int key) {}
void ui_action_map_clear_hotkey(int key) {}
int ui_action_is_valid(int action) { return 0; }

// UI Monitor Stubs - Minimal (uimon.c provides some?)
// Checking uimon.c in next step if collision. uimon.c is in HEADLESS_SRCS.
// Let's assume uimon.c implements them. remove these.


// UI Hotkeys Stubs
void ui_hotkeys_arch_install_by_map(void *map) {}
int ui_hotkeys_arch_keysym_from_arch(int key) { return 0; }
int ui_hotkeys_arch_keysym_to_arch(int key) { return 0; }
int ui_hotkeys_arch_modifier_to_arch(int mod) { return 0; }
int ui_hotkeys_arch_modmask_from_arch(int mod) { return 0; }
int ui_hotkeys_arch_modmask_to_arch(int mod) { return 0; }
void ui_hotkeys_arch_remove_by_map(void *map) {}
void ui_hotkeys_arch_shutdown(void) {}

// Thread/Exit Stubs
void archdep_vice_exit(int code) {
    NSLog(@"VICE exit with code %d", code);
    // DO NOT EXIT PROCESS. OpenEmu runs in process.
    // We should stop emulation.
    NSLog(@"[VICE] archdep_vice_exit called");
    VICEGameCore *core = [VICEGameCore current];
    if (core) {
        [core setEmulatorRunning:NO];
    }
}
void vice_macos_set_main_thread(void) {}
void vice_macos_set_vice_thread_priority(void) {}

void main_exit(void) {
    NSLog(@"[VICE] main_exit called via stub");
}

// Missing Userport/Printer Stubs (if not excluded)
void set_userport_flag(int val) {}

// Archdep Stubs
bool archdep_is_exiting(void) { return false; }
void archdep_thread_shutdown(void) {}
void archdep_thread_init(void) {}

// Gfxoutput Stub
int gfxoutput_init(void) { return 0; }
int gfxoutput_cmdline_options_init(void) { return 0; }
int gfxoutput_early_init(int help) { return 0; }
gfxoutputdrv_t *gfxoutput_get_driver(const char *drvname) { return NULL; }
int gfxoutput_resources_init(void) { return 0; }
void gfxoutput_shutdown(void) {}

// UI Actions Stubs
void ui_actions_init(void) {}
void ui_actions_shutdown(void) {}

// Userport Stubs
int userport_cmdline_options_init(void) { return 0; }
int userport_resources_init(void) { return 0; }
void userport_resources_shutdown(void) {}
int userport_dac_sound_chip_init(void) { return 0; }
int userport_digimax_sound_chip_init(void) { return 0; }
int userport_port_register(void) { return 0; }
int userport_device_register(void) { return 0; }
void *userport_get_device(int port) { return NULL; }
void userport_io_sim_set_pbx_out_lines(void) {}
void userport_powerup(void) {}
void userport_reset(void) {}
void userport_enable(void) {} // signature guess
int userport_snapshot_read_module(void *s) { return 0; }
int userport_snapshot_write_module(void *s) { return 0; }

// Userport IO Stubs
uint8_t read_userport_pa2(void) { return 0xff; }
uint8_t read_userport_pa3(void) { return 0xff; }
uint8_t read_userport_pbx(void) { return 0xff; }
uint8_t read_userport_sp1(void) { return 0xff; }
uint8_t read_userport_sp2(void) { return 0xff; }
void store_userport_pa2(uint8_t val) {}
void store_userport_pa3(uint8_t val) {}
void store_userport_pbx(uint8_t val) {}
void store_userport_sp1(uint8_t val) {}
void store_userport_sp2(uint8_t val) {}

@implementation VICEGameCore {
    pthread_mutex_t _videoMutex;
    NSString *_romPath;
}

+ (void)load {
    NSLog(@"[VICE] CLASS LOADED (dlopen success)");
    FILE *f = fopen("/tmp/vice_debug_load.txt", "a"); // Append to see multiple processes
    if (f) {
        fprintf(f, "VICEGameCore +load called at %ld by process: %s\n", time(NULL), getprogname());
        fclose(f);
    }
}

- (id)init {
    self = [super init];
    FILE *f = fopen("/tmp/vice_core_init.txt", "w");
    if (f) {
        fprintf(f, "VICEGameCore INIT called at %ld\n", time(NULL));
        fclose(f);
    }
    pthread_mutex_init(&_videoMutex, NULL);
    g_core = self;
    
    // Initialize video buffer default size
    int w = 384;
    int h = 272;
    self.videoBuffer = malloc(w * h * 4);
    memset(self.videoBuffer, 0, w * h * 4);
    
    return self;
}

- (NSUInteger)audioBitDepth { return 16; }

- (id)audioBufferAtIndex:(NSUInteger)index { return nil; }

- (void)dealloc
{
    NSLog(@"[VICE] Deallocating VICEGameCore");
    if (self.videoBuffer) free(self.videoBuffer);
    pthread_mutex_destroy(&_videoMutex);
}

# pragma mark - Execution

- (BOOL)loadFileAtPath:(NSString *)path error:(NSError **)error
{
    NSLog(@"[VICE] loadFileAtPath: entry with path: %@", path);
    _romPath = [path copy];
    
    NSLog(@"[VICE] loadFileAtPath: exit success");
    return YES;
}

- (void)setupEmulation
{
    NSLog(@"[VICE] setupEmulation");
    
    if (![[NSFileManager defaultManager] fileExistsAtPath:_romPath]) {
        NSLog(@"[VICE] Error: ROM file does not exist at path: %@", _romPath);
    } else {
        NSLog(@"[VICE] ROM exists at %@", _romPath);
    }
    
    static char *argv[] = {
        "x64",
        "-default", 
        "-logfile",
        "/tmp/vice.log",
        "-sounddev", "dummy",
        "-sound",
        "-autostart",
        NULL,
        NULL
    };
    
    argv[8] = (char *)[_romPath UTF8String];
    
    NSLog(@"[VICE] Spawning main_program thread with %s", argv[3]);
    
    [NSThread detachNewThreadWithBlock:^{
        NSLog(@"[VICE] Emulator thread started");
        self.emulatorRunning = YES;
        int result = main_program(9, argv);
        self.emulatorRunning = NO;
        NSLog(@"[VICE] main_program returned with %d", result);
    }];
}

// Duplicate executeFrame implementation check:
// earlier duplicate was removed. The valid one is now around line 320.


// Old executeFrame removed


- (void)stopEmulation
{
    NSLog(@"[VICE] stopEmulation called");
    
    // Signal emulator to quit cleanly
    g_quitEmulator = true;
    
    // Attempt to trigger main exit
    main_exit();
    
    // Wait for emulator thread to finish before shutting down resources
    int timeout = 0;
    while (self.emulatorRunning && timeout < 2000) {
        usleep(1000);
        timeout++;
    }
    if (self.emulatorRunning) {
        NSLog(@"[VICE] Warning: Emulator thread did not stop in time. Forcing sound suspend.");
    } else {
        NSLog(@"[VICE] Emulator thread stopped cleanly.");
    }
    // Suspend sound to prevent sound_flush during shutdown
    sound_suspend();
    // Shut down the machine
    machine_shutdown();
    // Finally close the sound subsystem
    sound_close();
    [super stopEmulation];
}


- (void)resetEmulation
{
    machine_trigger_reset(MACHINE_RESET_MODE_RESET_CPU);
}

- (void)executeFrame
{
    if (!self.emulatorRunning) {
        return;
    }
    
    // Reset frame finished flag
    _frameFinished = NO;
    
    // Wait for the emulator thread to signal frame completion
    // We use a busy-wait with small sleep for now, or just spin if tight.
    // Ideally use a condition variable.
    
    // Safety timeout to prevent freeze
    int safe = 0;
    while (!_frameFinished && safe < 1000) {
        usleep(1000); // 1ms
        safe++;
        
        // If the emulator thread pushes a frame, vsyncarch_postsync sets _frameFinished = YES
    }
    
    // Proceed to process video buffer (done in parent/caller usually picks up _videoBuffer)
}


# pragma mark - Video

- (const void *)getVideoBufferWithHint:(void *)hint
{
    // Return video buffer
    return _videoBuffer;
}

- (OEIntRect)screenRect
{
    return OEIntRectMake(0, 0, 384, 272); // Typical C64 rect, adjustable
}

- (OEIntSize)bufferSize
{
    return OEIntSizeMake(384, 272);
}

- (OEIntSize)aspectSize
{
    return OEIntSizeMake(4, 3);
}

- (GLenum)pixelFormat
{
    return GL_BGRA;
}

- (GLenum)pixelType
{
    return GL_UNSIGNED_INT_8_8_8_8_REV;
}

# pragma mark - Audio

- (double)audioSampleRate
{
    return 48000;
}

- (NSUInteger)channelCount
{
    return 2;
}

// Duplicate executeFrame removed

// VSync Arch Implementation
// Duplicate vsyncarch implementations removed
// Merged into the main implementation above

+ (id)current {
    return g_core;
}
- (void)setFrameFinished:(BOOL)finished {
    _frameFinished = finished;
}
// Duplicate init removed


#pragma mark - Input Handling

#pragma mark - OEC64SystemResponderClient

- (oneway void)mouseMovedAtPoint:(OEIntPoint)point {}
- (oneway void)leftMouseDownAtPoint:(OEIntPoint)point {}
- (oneway void)leftMouseUp {}
- (oneway void)rightMouseDownAtPoint:(OEIntPoint)point {}
- (oneway void)rightMouseUp {}
- (oneway void)swapJoysticks {}

- (oneway void)keyDown:(NSUInteger)keyCode {
    signed long key = [self mapKeyCode:(unsigned short)keyCode];
    if (key != -1) {
        keyboard_key_pressed(key, 0);
    }
}

- (oneway void)keyUp:(NSUInteger)keyCode {
    signed long key = [self mapKeyCode:(unsigned short)keyCode];
    if (key != -1) {
        keyboard_key_released(key, 0);
    }
}

- (signed long)mapKeyCode:(unsigned short)keyCode {
    // Mapping macOS keycodes to SDL keysyms (based on sdl_pos.vkm)
    switch(keyCode) {
        case 0x00: return 97; // A
        case 0x01: return 115; // S
        case 0x02: return 100; // D
        case 0x03: return 102; // F
        case 0x05: return 103; // G
        case 0x04: return 104; // H
        case 0x26: return 106; // J
        case 0x28: return 107; // K
        case 0x25: return 108; // L
        case 0x0B: return 98; // B
        case 0x08: return 99; // C
        case 0x0E: return 101; // E
        case 0x22: return 105; // I
        case 0x2D: return 110; // N
        case 0x24: return 13; // Return
        case 0x31: return 32; // Space
        case 0x7E: return 273; // Up
        case 0x7D: return 274; // Down
        case 0x7B: return 276; // Left
        case 0x7C: return 275; // Right
        case 0x33: return 8; // Delete (Inst/Del)
        // Add more keys as needed...
        default: return -1;
    }
}

// OEC64Button is defined in OEC64SystemResponderClient.h

- (oneway void)didPushC64Button:(OEC64Button)button forPlayer:(NSUInteger)player {
    if (player != 1) return; // Only P1 supported for now (mapped to Port 2)
    
    switch (button) {
        case OEC64JoystickUp:
            joystick_set_value_or(C64_JOYSTICK_PORT, JOYSTICK_DIRECTION_UP);
            break;
        case OEC64JoystickDown:
            joystick_set_value_or(C64_JOYSTICK_PORT, JOYSTICK_DIRECTION_DOWN);
            break;
        case OEC64JoystickLeft:
            joystick_set_value_or(C64_JOYSTICK_PORT, JOYSTICK_DIRECTION_LEFT);
            break;
        case OEC64JoystickRight:
            joystick_set_value_or(C64_JOYSTICK_PORT, JOYSTICK_DIRECTION_RIGHT);
            break;
        case OEC64ButtonFire:
            joystick_set_value_or(C64_JOYSTICK_PORT, 0x10);
            break;
        case OEC64ButtonJump:
            joystick_set_value_or(C64_JOYSTICK_PORT, JOYSTICK_DIRECTION_UP);
            break;
        case OEC64SwapJoysticks:
            // Todo: Implement port swap
            break;
        default:
            break;
    }
}

- (oneway void)didReleaseC64Button:(OEC64Button)button forPlayer:(NSUInteger)player {
    if (player != 1) return;

    switch (button) {
        case OEC64JoystickUp:
            joystick_set_value_and(C64_JOYSTICK_PORT, ~JOYSTICK_DIRECTION_UP);
            break;
        case OEC64JoystickDown:
            joystick_set_value_and(C64_JOYSTICK_PORT, ~JOYSTICK_DIRECTION_DOWN);
            break;
        case OEC64JoystickLeft:
            joystick_set_value_and(C64_JOYSTICK_PORT, ~JOYSTICK_DIRECTION_LEFT);
            break;
        case OEC64JoystickRight:
            joystick_set_value_and(C64_JOYSTICK_PORT, ~JOYSTICK_DIRECTION_RIGHT);
            break;
        case OEC64ButtonFire:
            joystick_set_value_and(C64_JOYSTICK_PORT, ~0x10);
            break;
        case OEC64ButtonJump:
            joystick_set_value_and(C64_JOYSTICK_PORT, ~JOYSTICK_DIRECTION_UP);
            break;
        default:
            break;
    }
}


@end
