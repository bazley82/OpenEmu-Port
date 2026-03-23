#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <dlfcn.h>

// OpenEmu SDK Headers
#import <OpenEmuBase/OEGeometry.h>
#import <OpenEmuBase/OEGameCore.h>
#import <OpenEmuBase/OEAudioBuffer.h>
#import <OpenEmuBase/OERingBuffer.h>
#import <pthread.h>

#if defined(__arm64__) || defined(__aarch64__)
#include <arm_neon.h>
#endif
#import <OpenEmuBase/OEGameCoreController.h>
#import <OpenEmuSystem/OEBindingMap.h>
#import <OpenEmuBase/OESystemResponderClient.h>

// Libretro
#import "libretro.h"
#import "OELibretroGameCore.h"

@interface OEGameCore (Internal)
- (void *)videoBufferAtIndex:(NSUInteger)index;
@end

#define RETRO_HW_CONTEXT_METAL 8

// We only define these if they are missing from libretro.h (usually older versions)
#ifndef RETRO_HW_RENDER_INTERFACE_METAL_VERSION
#define RETRO_HW_RENDER_INTERFACE_METAL 2
#define RETRO_HW_RENDER_INTERFACE_METAL_VERSION 1

struct retro_hw_render_interface_metal
{
    unsigned interface_type;
    unsigned interface_version;
    void *device;
    void *queue;
    void *video_callback;
};
#endif

#if DEBUG
static void OELogToFile(NSString *format, ...) {
    va_list args;
    va_start(args, format);
    NSString *msg = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);
    
    NSString *logLine = [NSString stringWithFormat:@"%@\n", msg];
    NSData *data = [logLine dataUsingEncoding:NSUTF8StringEncoding];
    
    NSString *logPath = @"/tmp/oe_libretro.log";
    NSFileHandle *handle = [NSFileHandle fileHandleForWritingAtPath:logPath];
    if (!handle) {
        [data writeToFile:logPath atomically:YES];
    } else {
        [handle seekToEndOfFile];
        [handle writeData:data];
        [handle closeFile];
    }
}
#else
#define OELogToFile(format, ...) do {} while (0)
#endif

static __unsafe_unretained OELibretroGameCore *_current;

@interface OELibretroGameCore ()

- (bool)environmentCallback:(unsigned)cmd data:(void *)data;
- (void)videoRefreshCallback:(const void *)data width:(unsigned)width height:(unsigned)height pitch:(size_t)pitch;
- (void)audioSampleCallback:(int16_t)left right:(int16_t)right;
- (size_t)audioSampleBatchCallback:(const int16_t *)data frames:(size_t)frames;
- (void)inputPollCallback;
- (int16_t)inputStateCallback:(unsigned)port device:(unsigned)device index:(unsigned)index id:(unsigned)id;
- (void)pushLibretroButton:(NSUInteger)button forPlayer:(NSUInteger)player;
- (void)releaseLibretroButton:(NSUInteger)button forPlayer:(NSUInteger)player;

@end

#pragma mark - Libretro Callbacks (C bridge)

static retro_proc_address_t retro_get_proc_address_cb(const char *sym)
{
    retro_proc_address_t addr = (retro_proc_address_t)dlsym(RTLD_DEFAULT, sym);
    if (!addr) {
        static void *gl_handle = NULL;
        if (!gl_handle) {
            gl_handle = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY | RTLD_LOCAL);
        }
        if (gl_handle) {
            addr = (retro_proc_address_t)dlsym(gl_handle, sym);
        }
    }
    if (!addr) {
        OELogToFile(@"[Libretro] Failed to find symbol: %s", sym);
    }
    return addr;
}

static void retro_log_cb(enum retro_log_level level, const char *fmt, ...)
{
}

static bool retro_environment_cb(unsigned cmd, void *data)
{
    return [_current environmentCallback:cmd data:data];
}

static void retro_video_refresh_cb(const void *data, unsigned width, unsigned height, size_t pitch)
{
    [_current videoRefreshCallback:data width:width height:height pitch:pitch];
}

static void retro_audio_sample_cb(int16_t left, int16_t right)
{
    [_current audioSampleCallback:left right:right];
}

static size_t retro_audio_sample_batch_cb(const int16_t *data, size_t frames)
{
    return [_current audioSampleBatchCallback:data frames:frames];
}

static void retro_input_poll_cb(void)
{
    [_current inputPollCallback];
}

static int16_t retro_input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    return [_current inputStateCallback:port device:device index:index id:id];
}

@implementation OELibretroGameCore

+ (void)load
{
    NSLog(@"[Libretro] OELibretroGameCore class loaded into runtime");
}

- (instancetype)init
{
    if ((self = [super init])) {
        _current = self;
        memset(_inputState, 0, sizeof(_inputState));
        _isHWContextActive = NO;
        _interfaceLoopCount = 0;
        _firstFrame = YES;
        _gamma = 1.0;
        _saturation = 1.0;
    }
    return self;
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
    NSMethodSignature *signature = [super methodSignatureForSelector:aSelector];
    if (!signature) {
        NSString *selName = NSStringFromSelector(aSelector);
        if ([selName hasPrefix:@"didPush"] || [selName hasPrefix:@"didRelease"]) {
            // (oneway void)method:(uint32_t)button forPlayer:(NSUInteger)player
            signature = [NSMethodSignature signatureWithObjCTypes:"Vv@:IQ"];
        } else {
            // Dummy method signature returning void to prevent unrecognized selector crashes
            signature = [NSMethodSignature signatureWithObjCTypes:"v@:"];
        }
    }
    return signature;
}

- (void)forwardInvocation:(NSInvocation *)anInvocation
{
    NSString *selName = NSStringFromSelector([anInvocation selector]);
    if ([selName hasPrefix:@"didPush"] && [selName hasSuffix:@"Button:forPlayer:"]) {
        uint32_t button = 0;
        NSUInteger player = 0;
        [anInvocation getArgument:&button atIndex:2];
        [anInvocation getArgument:&player atIndex:3];
        [self pushLibretroButton:(NSUInteger)button forPlayer:player];
        return;
    }
    
    if ([selName hasPrefix:@"didRelease"] && [selName hasSuffix:@"Button:forPlayer:"]) {
        uint32_t button = 0;
        NSUInteger player = 0;
        [anInvocation getArgument:&button atIndex:2];
        [anInvocation getArgument:&player atIndex:3];
        [self releaseLibretroButton:(NSUInteger)button forPlayer:player];
        return;
    }
    
    // Silently ignore unrecognized selectors
}

- (void)mouseMovedAtPoint:(OEIntPoint)point
{
    _mouseX = point.x;
    _mouseY = point.y;
}

- (void)mouseDownAtPoint:(OEIntPoint)point
{
    _mouseX = point.x;
    _mouseY = point.y;
    _mousePressed = YES;
}

- (void)mouseUpAtPoint
{
    _mousePressed = NO;
}

- (void)didTouchScreenPoint:(OEIntPoint)point
{
    _mouseX = point.x;
    _mouseY = point.y;
    _mousePressed = YES;
    
    OELogToFile(@"[Libretro] didTouchScreenPoint: (%d, %d)", _mouseX, _mouseY);
}

- (void)didReleaseTouch
{
    _mousePressed = NO;
    OELogToFile(@"[Libretro] didReleaseTouch");
}

- (void)dealloc
{
    if (_coreHandle) {
        dlclose(_coreHandle);
    }
}

#pragma mark - Symbols loading

- (BOOL)loadCore:(NSString *)corePath
{
    OELogToFile(@"[Libretro] Internal dlopen calling for %@", corePath);
    _coreHandle = dlopen([corePath UTF8String], RTLD_LAZY | RTLD_GLOBAL);
    if (!_coreHandle) {
        OELogToFile(@"[Libretro] Failed to load core at %@: %s", corePath, dlerror());
        NSLog(@"[Libretro] Failed to load core at %@: %s", corePath, dlerror());
        return NO;
    }
    OELogToFile(@"[Libretro] dlopen SUCCESS for core handle: %p", _coreHandle);
    
    #define LOAD_SYM(name) \
        _##name = (typeof(_##name))dlsym(_coreHandle, #name); \
        if (!_##name) { NSLog(@"[Libretro] Missing symbol: %s", #name); return NO; }

    LOAD_SYM(retro_init);
    LOAD_SYM(retro_deinit);
    LOAD_SYM(retro_api_version);
    LOAD_SYM(retro_get_system_info);
    LOAD_SYM(retro_get_system_av_info);
    LOAD_SYM(retro_set_controller_port_device);
    LOAD_SYM(retro_reset);
    LOAD_SYM(retro_run);
    LOAD_SYM(retro_serialize_size);
    LOAD_SYM(retro_serialize);
    LOAD_SYM(retro_unserialize);
    LOAD_SYM(retro_cheat_reset);
    LOAD_SYM(retro_cheat_set);
    LOAD_SYM(retro_load_game);
    LOAD_SYM(retro_unload_game);
    LOAD_SYM(retro_get_region);
    LOAD_SYM(retro_get_memory_data);
    LOAD_SYM(retro_get_memory_size);
    
    // Hardware Rendering
    _isHWContextActive = NO;
    
    // Optional
    _retro_load_game_special = (typeof(_retro_load_game_special))dlsym(_coreHandle, "retro_load_game_special");

    void (*set_env)(retro_environment_t) = (void (*)(retro_environment_t))dlsym(_coreHandle, "retro_set_environment");
    void (*set_video)(retro_video_refresh_t) = (void (*)(retro_video_refresh_t))dlsym(_coreHandle, "retro_set_video_refresh");
    void (*set_audio)(retro_audio_sample_t) = (void (*)(retro_audio_sample_t))dlsym(_coreHandle, "retro_set_audio_sample");
    void (*set_audio_batch)(retro_audio_sample_batch_t) = (void (*)(retro_audio_sample_batch_t))dlsym(_coreHandle, "retro_set_audio_sample_batch");
    void (*set_input_poll)(retro_input_poll_t) = (void (*)(retro_input_poll_t))dlsym(_coreHandle, "retro_set_input_poll");
    void (*set_input_state)(retro_input_state_t) = (void (*)(retro_input_state_t))dlsym(_coreHandle, "retro_set_input_state");

    if (set_env) set_env(retro_environment_cb);
    if (set_video) set_video(retro_video_refresh_cb);
    if (set_audio) set_audio(retro_audio_sample_cb);
    if (set_audio_batch) set_audio_batch(retro_audio_sample_batch_cb);
    if (set_input_poll) set_input_poll(retro_input_poll_cb);
    if (set_input_state) set_input_state(retro_input_state_cb);

    return YES;
}

#pragma mark - OpenEmu Execution

- (BOOL)loadFileAtPath:(NSString *)path error:(NSError **)error
{
    [[NSFileManager defaultManager] removeItemAtPath:@"/tmp/oe_libretro.log" error:nil];
    OELogToFile(@"[Libretro] loadFileAtPath: %@", path);
    NSString *bundlePath = [[NSBundle bundleForClass:[self class]] resourcePath];
    OELogToFile(@"[Libretro] bundlePath: %@", bundlePath);
    NSString *corePath = [bundlePath stringByAppendingPathComponent:@"libretro_core.dylib"];
    
    if (![[NSFileManager defaultManager] fileExistsAtPath:corePath]) {
        NSArray *contents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:bundlePath error:nil];
        for (NSString *file in contents) {
            if ([file hasSuffix:@".dylib"]) {
                corePath = [bundlePath stringByAppendingPathComponent:file];
                break;
            }
        }
    }
    
    OELogToFile(@"[Libretro] using corePath: %@", corePath);
    if (![self loadCore:corePath]) {
        OELogToFile(@"[Libretro] loadCore failed!");
        if (error) {
            *error = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreCouldNotStartCoreError userInfo:nil];
        }
        return NO;
    }
    
    _retro_init();
    OELogToFile(@"[Libretro] retro_init done.");
    
    struct retro_game_info game = {0};
    game.path = [path UTF8String];
    OELogToFile(@"[Libretro] game.path = %s", game.path);
    
    _retro_get_system_info(&_systemInfo);
    OELogToFile(@"[Libretro] System info: need_fullpath=%d", _systemInfo.need_fullpath);
    
    if (!_systemInfo.need_fullpath) {
        _gameData = [NSData dataWithContentsOfFile:path];
        if (_gameData) {
            game.data = [_gameData bytes];
            game.size = [_gameData length];
            OELogToFile(@"[Libretro] ROM loaded to memory, size: %zu bytes", game.size);
        } else {
            OELogToFile(@"[Libretro] Failed to load ROM data into memory for: %@", path);
            if (error) {
                *error = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreCouldNotLoadROMError userInfo:nil];
            }
            return NO;
        }
    }
    
    OELogToFile(@"[Libretro] Calling retro_load_game...");
    FILE *f_test = fopen([path UTF8String], "rb");
    if (f_test) {
        OELogToFile(@"[Libretro] fopen test SUCCESS");
        fclose(f_test);
    } else {
        OELogToFile(@"[Libretro] fopen test FAILED! errno = %d", errno);
    }
    
    if (!_retro_load_game(&game)) {
        return NO;
    }
    
    if (_isHWContextActive && _hwRenderCallback.context_reset) {
        _hwRenderCallback.context_reset();
    }
    
    // Explicitly enable pointer device for DS touch support
    _retro_set_controller_port_device(0, RETRO_DEVICE_POINTER);
    OELogToFile(@"[Libretro] Port 0 set to RETRO_DEVICE_POINTER");
    
    _retro_get_system_av_info(&_avInfo);
    
    // Check if we should use 2x software scaling to fill the screen better 
    // and avoid "hot pink" bars on low-res cores like Genesis.
    // Reverting as requested by user.
    _retro_get_system_av_info(&_avInfo);
    
    // Fix for cores that don't report geometry immediately
    if (_avInfo.geometry.base_width == 0 || _avInfo.geometry.base_height == 0) {
        _avInfo.geometry.base_width = 320;
        _avInfo.geometry.base_height = 240;
        _avInfo.geometry.max_width = 1024;
        _avInfo.geometry.max_height = 1024;
        _avInfo.geometry.aspect_ratio = 4.0 / 3.0;
        OELogToFile(@"[Libretro] Warning: Core reported 0x0 geometry. Using default fallback.");
    }

    OELogToFile(@"[Libretro] Core loaded. Geometry: base=%dx%d, max=%dx%d, aspect=%f", 
          _avInfo.geometry.base_width, _avInfo.geometry.base_height,
          _avInfo.geometry.max_width, _avInfo.geometry.max_height,
          _avInfo.geometry.aspect_ratio);
    
    return YES;
}

- (void)executeFrame
{
    if (_firstFrame) {
        _firstFrame = NO;
        if (_isHWContextActive && _hwRenderCallback.context_reset) {
            _hwRenderCallback.context_reset();
        }
    }
    _retro_run();
}

- (void)stopEmulation
{
    OELogToFile(@"[Libretro] stopEmulation called");
    if (_hwRenderCallback.context_destroy) {
        OELogToFile(@"[Libretro] Destroying hardware context");
        _hwRenderCallback.context_destroy();
    }
    
    OELogToFile(@"[Libretro] Calling _retro_unload_game");
    _retro_unload_game();
    OELogToFile(@"[Libretro] Calling _retro_deinit");
    _retro_deinit();
    
    _metalTexture = nil;
    
    if (_coreHandle) {
        OELogToFile(@"[Libretro] Closing core handle");
        dlclose(_coreHandle);
        _coreHandle = NULL;
    }
    
    OELogToFile(@"[Libretro] stopEmulation finished");
    [super stopEmulation];
}

- (void)resetEmulation
{
    _retro_reset();
}

#pragma mark - Libretro Callbacks Implementation

- (bool)environmentCallback:(unsigned)cmd data:(void *)data
{
    static unsigned last_cmd = 0xFFFFFFFF;
    if (cmd != last_cmd) {
        last_cmd = cmd;
    }

    switch (cmd) {
        case 35: // RETRO_ENVIRONMENT_SET_CONTROLLER_INFO
            OELogToFile(@"[Libretro] SET_CONTROLLER_INFO called");
            return true;
        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            *(bool*)data = true;
            return true;
            
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            *(const char**)data = [[self biosDirectoryPath] UTF8String];
            return true;
            
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
            *(const char**)data = [[self batterySavesDirectoryPath] UTF8String];
            return true;
            
        case RETRO_ENVIRONMENT_SET_HW_RENDER: {
            struct retro_hw_render_callback *cb = (struct retro_hw_render_callback *)data;
            if (cb) {
                _hwRenderCallback = *cb;
                _hwRenderCallback.get_proc_address = retro_get_proc_address_cb;
                _isHWContextActive = YES;
                _interfaceLoopCount = 0; 
                
                // For cores that expect an OpenGL context (like SwanStation)
                // but we only provide Metal, they might fail. 
                // We'll return true to try, but for now, we'll log it.
                OELogToFile(@"[Libretro] SET_HW_RENDER type: %d", cb->context_type);
                return true;
            }
            return false;
        }
            
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
            struct retro_log_callback *log = (struct retro_log_callback *)data;
            log->log = retro_log_cb;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE: {
            // This is used for Vulkan/Metal to negotiate specific context versions
            return true;
        }

        case RETRO_ENVIRONMENT_GET_VFS_INTERFACE: {
            struct retro_vfs_interface_info *vfs_info = (struct retro_vfs_interface_info *)data;
            if (vfs_info->required_interface_version <= 3) {
                // We'll provide a minimal VFS interface here if needed, 
                // but many cores will fall back to standard fopen if we return false.
                // For now, let's return false to see if the core handles it, 
                // or we can implement the full struct if necessary.
                return false;
            }
            return false;
        }

        case 55: { // RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE
            if (++_interfaceLoopCount > 10) {
                return false;
            }
            const struct retro_hw_render_interface **iface = (const struct retro_hw_render_interface **)data;
            id<MTLDevice> device = self.metalDevice;
            if (!device) return false;
            
            static struct retro_hw_render_interface_metal metal_iface;
            metal_iface.interface_type = 2; // RETRO_HW_RENDER_INTERFACE_METAL
            metal_iface.interface_version = 1; // RETRO_HW_RENDER_INTERFACE_METAL_VERSION
            metal_iface.device = (__bridge void *)device;
            
            static id<MTLCommandQueue> _q = nil;
            if (!_q) _q = [device newCommandQueue];
            metal_iface.queue = (__bridge void *)_q;
            
            metal_iface.video_callback = NULL;
            *iface = (const struct retro_hw_render_interface *)&metal_iface;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
            _pixelFormat = *(const enum retro_pixel_format *)data;
            OELogToFile(@"[Libretro] SET_PIXEL_FORMAT: %d", _pixelFormat);
            return true;
        }

        case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER: {
            unsigned *type = (unsigned *)data;
            *type = 3; // RETRO_HW_CONTEXT_OPENGL_CORE
            return true;
        }
        case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: {
            if (data) {
                unsigned *version = (unsigned *)data;
                *version = 2;
            }
            return true;
        }
        case RETRO_ENVIRONMENT_SET_GEOMETRY: {
            const struct retro_game_geometry *geometry = (const struct retro_game_geometry *)data;
            _avInfo.geometry = *geometry;
            OELogToFile(@"[Libretro] SET_GEOMETRY: %dx%d", _avInfo.geometry.base_width, _avInfo.geometry.base_height);
            return true;
        }
        case RETRO_ENVIRONMENT_GET_LANGUAGE: {
            if (data) {
                unsigned *lang = (unsigned *)data;
                *lang = 0; // RETRO_LANGUAGE_ENGLISH
            }
            return true;
        }
        case 65576: // RETRO_ENVIRONMENT_GET_VFS_INTERFACE
            return false;
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
            if (data) {
                bool *update = (bool *)data;
                *update = false;
            }
            return true;
        }
        case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: {
            // We cannot dynamically update _avInfo because OpenEmu relies on bufferSize and bytesPerRow remaining constant
            // to match the statically allocated IOSurface.
            return true;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE: {
            struct retro_variable *var = (struct retro_variable *)data;
            if (var && var->key) {
                OELogToFile(@"[Libretro] GET_VARIABLE: %s", var->key);
                
                if (strcmp(var->key, "desmume_pointer_type") == 0) {
                    var->value = "pointer";
                    OELogToFile(@"[Libretro] Returning 'pointer' for desmume_pointer_type");
                    return true;
                }
                if (strcmp(var->key, "desmume_pointer_mouse") == 0) {
                    var->value = "disabled";
                    OELogToFile(@"[Libretro] Returning 'disabled' for desmume_pointer_mouse");
                    return true;
                }
                if (strcmp(var->key, "desmume_pointer_device_l") == 0) {
                    var->value = "none";
                    OELogToFile(@"[Libretro] Returning 'none' for desmume_pointer_device_l");
                    return true;
                }
                if (strcmp(var->key, "desmume_pointer_device_r") == 0) {
                    var->value = "none";
                    OELogToFile(@"[Libretro] Returning 'none' for desmume_pointer_device_r");
                    return true;
                }
                
                // N64 Software Rendering Fallback
                if (strcmp(var->key, "mupen64plus-rdp-plugin") == 0) {
                    var->value = "angrylion";
                    OELogToFile(@"[Libretro] Returning 'angrylion' for mupen64plus-rdp-plugin");
                    return true;
                }
                if (strcmp(var->key, "mupen64plus-rsp-plugin") == 0) {
                    var->value = "hle";
                    OELogToFile(@"[Libretro] Returning 'hle' for mupen64plus-rsp-plugin");
                    return true;
                }
                
                if (strcmp(var->key, "mupen64plus-internal_resolution") == 0) {
                    if (self.resolutionScale < 0.125) var->value = "320x240";
                    else if (self.resolutionScale < 0.375) var->value = "640x480";
                    else if (self.resolutionScale < 0.625) var->value = "960x720";
                    else var->value = "1280x960";
                    OELogToFile(@"[Libretro] Internal resolution requested: returning %s for scale %f", var->value, self.resolutionScale);
                    return true;
                }
                if (strcmp(var->key, "ppsspp_backend") == 0) {
                    var->value = "opengl";
                    OELogToFile(@"[Libretro] Forced ppsspp_backend to %s", var->value);
                    return true;
                }
                if (strcmp(var->key, "ppsspp_software_rendering") == 0) {
                    var->value = "disabled";
                    OELogToFile(@"[Libretro] PPSSPP software rendering requested: returning %s", var->value);
                    return true;
                }
                if (strcmp(var->key, "ppsspp_internal_resolution") == 0) {
                    if (self.resolutionScale < 0.125) var->value = "480x272";
                    else if (self.resolutionScale < 0.375) var->value = "960x544";
                    else if (self.resolutionScale < 0.625) var->value = "1440x816";
                    else var->value = "1920x1088";
                    OELogToFile(@"[Libretro] PPSSPP Internal resolution requested (scale %f): returning %s", self.resolutionScale, var->value);
                    return true;
                }
                if (strcmp(var->key, "swanstation_GPU_Renderer") == 0 || 
                    strcmp(var->key, "swanstation.Renderer") == 0 ||
                    strcmp(var->key, "renderer_selection") == 0) {
                    var->value = "Software";
                    OELogToFile(@"[Libretro] Forced SwanStation to Software Renderer (Key: %s)", var->key);
                    return true;
                }
                if (strcmp(var->key, "swanstation_GPU_ResolutionScale") == 0 ||
                    strcmp(var->key, "swanstation.ResolutionScale") == 0) {
                    var->value = "1";
                    return true;
                }
                if (strncmp(var->key, "swanstation_BIOS_Path", 21) == 0) {
                     return false;
                }
                if (strcmp(var->key, "mupen64plus-ThreadedRenderer") == 0) {
                    var->value = "enabled";
                    OELogToFile(@"[Libretro] Returning 'enabled' for mupen64plus-ThreadedRenderer");
                    return true;
                }
            }
            return false;
        }

        case RETRO_ENVIRONMENT_SET_VARIABLES:
            return true;

        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
            return true;

        case RETRO_ENVIRONMENT_SHUTDOWN:
            [self stopEmulation];
            return true;
            
        default:
            return false;
    }
}

- (OEGameCoreRendering)gameCoreRendering
{
    if (!_isHWContextActive) {
        return OEGameCoreRenderingBitmap;
    }
    
    if (_hwRenderCallback.context_type == 1 /* RETRO_HW_CONTEXT_OPENGL */) {
        return (OEGameCoreRendering)1; // OEGameCoreRenderingOpenGL2
    } else if (_hwRenderCallback.context_type == 3 /* RETRO_HW_CONTEXT_OPENGL_CORE */) {
        return (OEGameCoreRendering)2; // OEGameCoreRenderingOpenGL3
    }
    return (OEGameCoreRendering)2; // Default to 2 (OpenGL3) as in verified stable commit 8fa7993
}

- (void)videoRefreshCallback:(const void *)data width:(unsigned)width height:(unsigned)height pitch:(size_t)pitch
{
    if (data == RETRO_HW_FRAME_BUFFER_VALID) {
        id renderDelegate = [self valueForKey:@"renderDelegate"];
        if ([renderDelegate respondsToSelector:@selector(didRenderFrameOnAlternateThread)]) {
            [renderDelegate performSelector:@selector(didRenderFrameOnAlternateThread)];
        }
        return;
    }
    
    if (data == NULL) return;
    
    if (_currentWidth != width || _currentHeight != height) {
        _currentWidth = width;
        _currentHeight = height;
    }
    
    void *dest = (void *)[self videoBuffer];
    if (dest && data != dest) {
        OEIntSize maxSize = [self bufferSize];
        unsigned copyW = MIN(width, (unsigned)maxSize.width);
        unsigned copyH = MIN(height, (unsigned)maxSize.height);
        
        size_t outPitch = [self bytesPerRow];
        size_t inPitch = pitch;
        
        uint32_t bytesPerPixel = (_pixelFormat == RETRO_PIXEL_FORMAT_XRGB8888) ? 4 : 2;
        if (inPitch == 0) inPitch = width * bytesPerPixel;
        
        if (_pixelFormat == RETRO_PIXEL_FORMAT_XRGB8888) {
            if (inPitch == outPitch && inPitch == width * 4) {
                memcpy(dest, data, inPitch * copyH);
            } else {
                for (int y = 0; y < copyH; y++) {
                    memcpy((uint8_t *)dest + y * outPitch, (uint8_t *)data + y * inPitch, copyW * 4);
                }
            }
        } else if (_pixelFormat == RETRO_PIXEL_FORMAT_RGB565) {
            const uint16_t *src_row = (const uint16_t *)data;
            uint32_t *dst_row = (uint32_t *)dest;
            for (int y = 0; y < copyH; y++) {
                int x = 0;
#if defined(__arm64__) || defined(__aarch64__)
                for (; x <= (int)copyW - 8; x += 8) {
                    uint16x8_t pixels = vld1q_u16(&src_row[x]);
                    
                    // RGB565: RRRRRGGGGGGBBBBB
                    // R: (pix >> 11) & 0x1F  -> 5 bits
                    // G: (pix >> 5) & 0x3F   -> 6 bits
                    // B: (pix) & 0x1F        -> 5 bits
                    
                    uint16x8_t r16 = vshrq_n_u16(pixels, 11);
                    uint16x8_t g16 = vshrq_n_u16(vandq_u16(pixels, vdupq_n_u16(0x07E0)), 5);
                    uint16x8_t b16 = vandq_u16(pixels, vdupq_n_u16(0x001F));
                    
                    // Expand 5/6 bits to 8 bits: (v << (8-bits)) | (v >> (2*bits-8))
                    // R(5->8): (r << 3) | (r >> 2)
                    // G(6->8): (g << 2) | (g >> 4)
                    // B(5->8): (b << 3) | (b >> 2)
                    
                    uint16x8_t r8 = vorrq_u16(vshlq_n_u16(r16, 3), vshrq_n_u16(r16, 2));
                    uint16x8_t g8 = vorrq_u16(vshlq_n_u16(g16, 2), vshrq_n_u16(g16, 4));
                    uint16x8_t b8 = vorrq_u16(vshlq_n_u16(b16, 3), vshrq_n_u16(b16, 2));
                    
                    // Pack into 32-bit: 0xFF000000 | (r8 << 16) | (g8 << 8) | b8
                    // We need to split into two uint32x4_t
                    uint8x16_t a8_v = vdupq_n_u8(0xFF);
                    
                    // Extract low and high halves
                    uint8x8_t rL = vmovn_u16(r8);
                    uint8x8_t gL = vmovn_u16(g8);
                    uint8x8_t b8L = vmovn_u16(b8);
                    
                    // (unrolled for stability)
                    dst_row[x+0] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 0)) << 16) | (((uint32_t)vget_lane_u8(gL, 0)) << 8) | ((uint32_t)vget_lane_u8(b8L, 0));
                    dst_row[x+1] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 1)) << 16) | (((uint32_t)vget_lane_u8(gL, 1)) << 8) | ((uint32_t)vget_lane_u8(b8L, 1));
                    dst_row[x+2] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 2)) << 16) | (((uint32_t)vget_lane_u8(gL, 2)) << 8) | ((uint32_t)vget_lane_u8(b8L, 2));
                    dst_row[x+3] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 3)) << 16) | (((uint32_t)vget_lane_u8(gL, 3)) << 8) | ((uint32_t)vget_lane_u8(b8L, 3));
                    dst_row[x+4] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 4)) << 16) | (((uint32_t)vget_lane_u8(gL, 4)) << 8) | ((uint32_t)vget_lane_u8(b8L, 4));
                    dst_row[x+5] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 5)) << 16) | (((uint32_t)vget_lane_u8(gL, 5)) << 8) | ((uint32_t)vget_lane_u8(b8L, 5));
                    dst_row[x+6] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 6)) << 16) | (((uint32_t)vget_lane_u8(gL, 6)) << 8) | ((uint32_t)vget_lane_u8(b8L, 6));
                    dst_row[x+7] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 7)) << 16) | (((uint32_t)vget_lane_u8(gL, 7)) << 8) | ((uint32_t)vget_lane_u8(b8L, 7));
                }
#endif
                for (; x < (int)copyW; x++) {
                    uint16_t pix = src_row[x];
                    uint32_t r = (pix >> 11) & 0x1F;
                    uint32_t g = (pix >> 5) & 0x3F;
                    uint32_t b = (pix) & 0x1F;
                    dst_row[x] = (0xFF << 24) | ((r << 3 | r >> 2) << 16) | ((g << 2 | g >> 4) << 8) | (b << 3 | b >> 2);
                }
                src_row = (const uint16_t *)((const uint8_t *)src_row + inPitch);
                dst_row = (uint32_t *)((uint8_t *)dst_row + outPitch);
            }
        } else if (_pixelFormat == RETRO_PIXEL_FORMAT_0RGB1555) {
            const uint16_t *src_row = (const uint16_t *)data;
            uint32_t *dst_row = (uint32_t *)dest;
            for (int y = 0; y < copyH; y++) {
                int x = 0;
#if defined(__arm64__) || defined(__aarch64__)
                for (; x <= (int)copyW - 8; x += 8) {
                    uint16x8_t pixels = vld1q_u16(&src_row[x]);
                    
                    // 0RGB1555: 0RRRRRGGGGGBBBBB
                    // R: (pix >> 10) & 0x1F
                    // G: (pix >> 5) & 0x1F
                    // B: (pix) & 0x1F
                    
                    uint16x8_t r16 = vshrq_n_u16(vandq_u16(pixels, vdupq_n_u16(0x7C00)), 10);
                    uint16x8_t g16 = vshrq_n_u16(vandq_u16(pixels, vdupq_n_u16(0x03E0)), 5);
                    uint16x8_t b16 = vandq_u16(pixels, vdupq_n_u16(0x001F));
                    
                    uint16x8_t r8 = vorrq_u16(vshlq_n_u16(r16, 3), vshrq_n_u16(r16, 2));
                    uint16x8_t g8 = vorrq_u16(vshlq_n_u16(g16, 3), vshrq_n_u16(g16, 2));
                    uint16x8_t b8 = vorrq_u16(vshlq_n_u16(b16, 3), vshrq_n_u16(b16, 2));
                    
                    uint8x8_t rL = vmovn_u16(r8);
                    uint8x8_t gL = vmovn_u16(g8);
                    uint8x8_t b8L = vmovn_u16(b8);
                    
                    dst_row[x+0] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 0)) << 16) | (((uint32_t)vget_lane_u8(gL, 0)) << 8) | ((uint32_t)vget_lane_u8(b8L, 0));
                    dst_row[x+1] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 1)) << 16) | (((uint32_t)vget_lane_u8(gL, 1)) << 8) | ((uint32_t)vget_lane_u8(b8L, 1));
                    dst_row[x+2] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 2)) << 16) | (((uint32_t)vget_lane_u8(gL, 2)) << 8) | ((uint32_t)vget_lane_u8(b8L, 2));
                    dst_row[x+3] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 3)) << 16) | (((uint32_t)vget_lane_u8(gL, 3)) << 8) | ((uint32_t)vget_lane_u8(b8L, 3));
                    dst_row[x+4] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 4)) << 16) | (((uint32_t)vget_lane_u8(gL, 4)) << 8) | ((uint32_t)vget_lane_u8(b8L, 4));
                    dst_row[x+5] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 5)) << 16) | (((uint32_t)vget_lane_u8(gL, 5)) << 8) | ((uint32_t)vget_lane_u8(b8L, 5));
                    dst_row[x+6] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 6)) << 16) | (((uint32_t)vget_lane_u8(gL, 6)) << 8) | ((uint32_t)vget_lane_u8(b8L, 6));
                    dst_row[x+7] = (0xFF << 24) | (((uint32_t)vget_lane_u8(rL, 7)) << 16) | (((uint32_t)vget_lane_u8(gL, 7)) << 8) | ((uint32_t)vget_lane_u8(b8L, 7));
                }
#endif
                for (; x < (int)copyW; x++) {
                    uint16_t pix = src_row[x];
                    uint32_t r = (pix >> 10) & 0x1F;
                    uint32_t g = (pix >> 5) & 0x1F;
                    uint32_t b = (pix) & 0x1F;
                    dst_row[x] = (0xFF << 24) | ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
                }
                src_row = (const uint16_t *)((const uint8_t *)src_row + inPitch);
                dst_row = (uint32_t *)((uint8_t *)dst_row + outPitch);
            }
        }
    }
}

- (void)audioSampleCallback:(int16_t)left right:(int16_t)right
{
    int16_t samples[2] = {left, right};
    [[self audioBufferAtIndex:0] write:(const uint8_t *)samples maxLength:4];
}

- (size_t)audioSampleBatchCallback:(const int16_t *)data frames:(size_t)frames
{
    // OERingBuffer write: returns 1 (true) on success, 0 on failure.
    // Libretro expects the number of frames written.
    NSUInteger result = [[self audioBufferAtIndex:0] write:(const uint8_t *)data maxLength:frames * 4];
    return result ? frames : 0;
}

- (void)inputPollCallback
{
}

- (int16_t)inputStateCallback:(unsigned)port device:(unsigned)device index:(unsigned)index id:(unsigned)id
{
    if (port >= 8) return 0;

    if (device == RETRO_DEVICE_JOYPAD) {
        if (id < 16) {
            return _inputState[port][id];
        }
    } else {
        static unsigned last_device = 0xFF;
        static unsigned last_index = 0xFF;
        if (device != last_device || index != last_index) {
            last_device = device;
            last_index = index;
        }
    }

    if (device == RETRO_DEVICE_ANALOG) {
        // Fallback: many DS cores use the analog stick for the stylus if pointer isn't enabled/ready.
        // Usually Port 0, Index 1 (Right Stick) or Index 0 (Left Stick).
        if (id == RETRO_DEVICE_ID_ANALOG_X) {
            return (int16_t)((_mouseX * 65534.0f / _currentWidth) - 32767.0f);
        } else if (id == RETRO_DEVICE_ID_ANALOG_Y) {
            return (int16_t)((_mouseY * 65534.0f / _currentHeight) - 32767.0f);
        }
    }

    if (device == RETRO_DEVICE_POINTER) {
        if (_mousePressed && id == RETRO_DEVICE_ID_POINTER_PRESSED) {
        }
        switch (id) {
            case RETRO_DEVICE_ID_POINTER_X: {
                if (_currentWidth == 0) return 0;
                int16_t res = (int16_t)((_mouseX * 65534.0f / _currentWidth) - 32767.0f);
                return res;
            }
            case RETRO_DEVICE_ID_POINTER_Y: {
                if (_currentHeight == 0) return 0;
                int16_t res = (int16_t)((_mouseY * 65534.0f / _currentHeight) - 32767.0f);
                return res;
            }
            case RETRO_DEVICE_ID_POINTER_PRESSED:
                return _mousePressed ? 1 : 0;
        }
    }
    return 0;
}

- (uint32_t)_retroIDForButton:(NSUInteger)key system:(NSString *)systemID
{
    
    if ([systemID isEqualToString:@"openemu.system.snes"]) {
        // SNES Mapping
        switch (key) {
            case 0: return RETRO_DEVICE_ID_JOYPAD_UP;
            case 1: return RETRO_DEVICE_ID_JOYPAD_DOWN;
            case 2: return RETRO_DEVICE_ID_JOYPAD_LEFT;
            case 3: return RETRO_DEVICE_ID_JOYPAD_RIGHT;
            case 4: return RETRO_DEVICE_ID_JOYPAD_A;
            case 5: return RETRO_DEVICE_ID_JOYPAD_B;
            case 6: return RETRO_DEVICE_ID_JOYPAD_X;
            case 7: return RETRO_DEVICE_ID_JOYPAD_Y;
            case 8: return RETRO_DEVICE_ID_JOYPAD_L;
            case 9: return RETRO_DEVICE_ID_JOYPAD_R;
            case 10: return RETRO_DEVICE_ID_JOYPAD_START;
            case 11: return RETRO_DEVICE_ID_JOYPAD_SELECT;
            default: return 0xFFFF;
        }
    } else if ([systemID isEqualToString:@"openemu.system.psx"]) {
        // PS1 Mapping
        switch (key) {
            case 0: return RETRO_DEVICE_ID_JOYPAD_UP;
            case 1: return RETRO_DEVICE_ID_JOYPAD_DOWN;
            case 2: return RETRO_DEVICE_ID_JOYPAD_LEFT;
            case 3: return RETRO_DEVICE_ID_JOYPAD_RIGHT;
            case 4: return RETRO_DEVICE_ID_JOYPAD_X; // Triangle -> X
            case 5: return RETRO_DEVICE_ID_JOYPAD_A; // Circle -> A
            case 6: return RETRO_DEVICE_ID_JOYPAD_B; // Cross -> B
            case 7: return RETRO_DEVICE_ID_JOYPAD_Y; // Square -> Y
            case 8: return RETRO_DEVICE_ID_JOYPAD_L;
            case 9: return RETRO_DEVICE_ID_JOYPAD_L2;
            case 10: return RETRO_DEVICE_ID_JOYPAD_L3;
            case 11: return RETRO_DEVICE_ID_JOYPAD_R;
            case 12: return RETRO_DEVICE_ID_JOYPAD_R2;
            case 13: return RETRO_DEVICE_ID_JOYPAD_R3;
            case 14: return RETRO_DEVICE_ID_JOYPAD_START;
            case 15: return RETRO_DEVICE_ID_JOYPAD_SELECT;
            default: return 0xFFFF;
        }
    } else if ([systemID isEqualToString:@"openemu.system.n64"]) {
        // N64 Mapping (Simplified)
        switch (key) {
            case 0: return RETRO_DEVICE_ID_JOYPAD_UP; // DPad
            case 1: return RETRO_DEVICE_ID_JOYPAD_DOWN;
            case 2: return RETRO_DEVICE_ID_JOYPAD_LEFT;
            case 3: return RETRO_DEVICE_ID_JOYPAD_RIGHT;
            case 8: return RETRO_DEVICE_ID_JOYPAD_A;
            case 9: return RETRO_DEVICE_ID_JOYPAD_B;
            case 10: return RETRO_DEVICE_ID_JOYPAD_L;
            case 11: return RETRO_DEVICE_ID_JOYPAD_R;
            case 12: return RETRO_DEVICE_ID_JOYPAD_L2; // Z -> L2 typically
            case 13: return RETRO_DEVICE_ID_JOYPAD_START;
            default: return 0xFFFF;
        }
    } else if ([systemID isEqualToString:@"openemu.system.sg"]) {
        // Sega Genesis Mapping
        switch (key) {
            case 0: return RETRO_DEVICE_ID_JOYPAD_UP;
            case 1: return RETRO_DEVICE_ID_JOYPAD_DOWN;
            case 2: return RETRO_DEVICE_ID_JOYPAD_LEFT;
            case 3: return RETRO_DEVICE_ID_JOYPAD_RIGHT;
            case 4: return RETRO_DEVICE_ID_JOYPAD_B; // A -> B
            case 5: return RETRO_DEVICE_ID_JOYPAD_A; // B -> A
            case 6: return RETRO_DEVICE_ID_JOYPAD_X; // C -> X
            case 7: return RETRO_DEVICE_ID_JOYPAD_Y; // X -> Y
            case 8: return RETRO_DEVICE_ID_JOYPAD_L; // Y -> L
            case 9: return RETRO_DEVICE_ID_JOYPAD_R; // Z -> R
            case 10: return RETRO_DEVICE_ID_JOYPAD_START;
            case 11: return RETRO_DEVICE_ID_JOYPAD_SELECT; // Mode -> Select
            default: return 0xFFFF;
        }
    } else if ([systemID isEqualToString:@"openemu.system.nds"]) {
        // Nintendo DS Mapping
        switch (key) {
            case 0: return RETRO_DEVICE_ID_JOYPAD_UP;
            case 1: return RETRO_DEVICE_ID_JOYPAD_DOWN;
            case 2: return RETRO_DEVICE_ID_JOYPAD_LEFT;
            case 3: return RETRO_DEVICE_ID_JOYPAD_RIGHT;
            case 4: return RETRO_DEVICE_ID_JOYPAD_A;
            case 5: return RETRO_DEVICE_ID_JOYPAD_B;
            case 6: return RETRO_DEVICE_ID_JOYPAD_X;
            case 7: return RETRO_DEVICE_ID_JOYPAD_Y;
            case 8: return RETRO_DEVICE_ID_JOYPAD_L;
            case 9: return RETRO_DEVICE_ID_JOYPAD_R;
            case 10: return RETRO_DEVICE_ID_JOYPAD_START;
            case 11: return RETRO_DEVICE_ID_JOYPAD_SELECT;
            default: return 0xFFFF;
        }
    } else if ([systemID isEqualToString:@"openemu.system.psp"]) {
        // PSP Mapping
        switch (key) {
            case 0: return RETRO_DEVICE_ID_JOYPAD_UP;
            case 1: return RETRO_DEVICE_ID_JOYPAD_DOWN;
            case 2: return RETRO_DEVICE_ID_JOYPAD_LEFT;
            case 3: return RETRO_DEVICE_ID_JOYPAD_RIGHT;
            case 4: return RETRO_DEVICE_ID_JOYPAD_B; // Cross -> B
            case 5: return RETRO_DEVICE_ID_JOYPAD_A; // Circle -> A
            case 6: return RETRO_DEVICE_ID_JOYPAD_Y; // Square -> Y
            case 7: return RETRO_DEVICE_ID_JOYPAD_X; // Triangle -> X
            case 8: return RETRO_DEVICE_ID_JOYPAD_L;
            case 9: return RETRO_DEVICE_ID_JOYPAD_R;
            case 10: return RETRO_DEVICE_ID_JOYPAD_START;
            case 11: return RETRO_DEVICE_ID_JOYPAD_SELECT;
            default: return 0xFFFF;
        }
    }
    
    // Default: return key as is (might work for simple systems)
    return (uint32_t)key;
}

#pragma mark - Libretro Input Mapping

- (void)pushLibretroButton:(NSUInteger)button forPlayer:(NSUInteger)player
{
    player = player > 0 ? player - 1 : 0;
    if (player >= 8) return;
    
    NSString *systemID = self.systemIdentifier;
    uint32_t retro_id = [self _retroIDForButton:button system:systemID];
    if (retro_id < 16) {
        _inputState[player][retro_id] = 1;
    }
}

- (void)releaseLibretroButton:(NSUInteger)button forPlayer:(NSUInteger)player
{
    player = player > 0 ? player - 1 : 0;
    if (player >= 8) return;
    
    NSString *systemID = self.systemIdentifier;
    uint32_t retro_id = [self _retroIDForButton:button system:systemID];
    if (retro_id < 16) {
        _inputState[player][retro_id] = 0;
    }
}

#pragma mark - OpenEmu Video/Audio Properties

- (OEIntSize)bufferSize
{
    int width = _avInfo.geometry.max_width ?: _avInfo.geometry.base_width;
    int height = _avInfo.geometry.max_height ?: _avInfo.geometry.base_height;
    
    if (width <= 0) width = 640;
    if (height <= 0) height = 480;
    
    return (OEIntSize){(int32_t)width, (int32_t)height};
}

- (OEIntRect)screenRect
{
    int width = _currentWidth > 0 ? _currentWidth : _avInfo.geometry.base_width;
    int height = _currentHeight > 0 ? _currentHeight : _avInfo.geometry.base_height;
    
    if (width <= 0) width = 640;
    if (height <= 0) height = 480;
    
    return (OEIntRect){{(int32_t)0, (int32_t)0}, {(int32_t)width, (int32_t)height}};
}

- (OEIntSize)aspectSize
{
    float ratio = _avInfo.geometry.aspect_ratio;
    
    // Sega Genesis / Mega Drive (openemu.system.sg)
    // Force 4:3 display aspect ratio to match original OpenEmu behavior 
    // and eliminate horizontal letterboxing bars (Genesis 320x224 is natively ~1.43).
    if ([self.systemIdentifier isEqualToString:@"openemu.system.sg"]) {
        ratio = 4.0 / 3.0;
    }
    
    if (ratio <= 0) {
        OEIntRect rect = [self screenRect];
        ratio = (float)rect.size.width / (float)rect.size.height;
    }
    return (OEIntSize){(int32_t)(ratio * 1000), (int32_t)1000};
}

- (NSInteger)bytesPerRow
{
    OEIntSize size = [self bufferSize];
    int bpp = [self pixelType] == OEPixelType_UNSIGNED_INT_8_8_8_8_REV ? 4 : 2;
    return size.width * bpp;
}

- (double)audioSampleRate
{
    return _avInfo.timing.sample_rate;
}

- (NSUInteger)channelCount
{
    return 2;
}

- (NSUInteger)audioBitDepth
{
    return 16;
}

- (NSTimeInterval)frameInterval
{
    return _avInfo.timing.fps > 0 ? _avInfo.timing.fps : 60.0;
}

- (uint32_t)pixelFormat
{
    return OEPixelFormat_BGRA;
}

- (const void *)videoBuffer
{
    return _videoBuffer ?: _rendererBuffer;
}

- (const void *)getVideoBufferWithHint:(void *)hint
{
    if (!hint) {
        OEIntSize size = [self bufferSize];
        size_t needed = size.height * [self bytesPerRow];
        if (needed > _videoBufferSize || !_videoBuffer) {
            if (_videoBuffer) free(_videoBuffer);
            _videoBuffer = malloc(needed > 0 ? needed : 1024*1024 * 4);
            _videoBufferSize = needed > 0 ? needed : 1024*1024 * 4;
            
            // Clear to opaque black (0xFF000000 in LE is 0x00, 0x00, 0x00, 0xFF)
            // We can use a 32-bit loop for this.
            uint32_t *p = (uint32_t *)_videoBuffer;
            size_t count = _videoBufferSize / 4;
            for (size_t i = 0; i < count; i++) p[i] = 0xFF000000;
            
            OELogToFile(@"[Libretro] Allocated and cleared video buffer: %zu bytes", _videoBufferSize);
        }
        _rendererBuffer = _videoBuffer;
        return _videoBuffer;
    }
    _rendererBuffer = hint;
    return hint;
}

- (void)saveStateToFileAtPath:(NSString *)fileName completionHandler:(void (^)(BOOL, NSError *))block
{
    size_t size = _retro_serialize_size();
    if (size == 0) {
        block(NO, nil);
        return;
    }
    
    void *data = malloc(size);
    if (_retro_serialize(data, size)) {
        NSData *saveData = [NSData dataWithBytesNoCopy:data length:size];
        BOOL success = [saveData writeToFile:fileName atomically:YES];
        block(success, nil);
    } else {
        free(data);
        block(NO, nil);
    }
}

- (void)loadStateFromFileAtPath:(NSString *)fileName completionHandler:(void (^)(BOOL, NSError *))block
{
    NSData *saveData = [NSData dataWithContentsOfFile:fileName];
    if (!saveData) {
        block(NO, nil);
        return;
    }
    
    BOOL success = _retro_unserialize([saveData bytes], [saveData length]);
    block(success, nil);
}

- (uint32_t)pixelType
{
    return OEPixelType_UNSIGNED_INT_8_8_8_8_REV;
}



- (void)createMetalTextureWithDevice:(id<MTLDevice>)device
{
    if (_hwRenderCallback.context_type != RETRO_HW_CONTEXT_METAL) {
        return;
    }
    
    MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                          width:_avInfo.geometry.max_width
                                                                                         height:_avInfo.geometry.max_height
                                                                                      mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModePrivate;
    
    _metalTexture = [device newTextureWithDescriptor:descriptor];
    
    if (_hwRenderCallback.context_reset) {
        _hwRenderCallback.context_reset();
    }
    
    _isHWContextActive = YES;
}

- (id<MTLTexture>)metalTexture
{
    return _metalTexture;
}

- (id<MTLDevice>)metalDevice
{
    return [super metalDevice];
}

@synthesize resolutionScale = _resolutionScale;
@synthesize gamma = _gamma;
@synthesize saturation = _saturation;

- (void)setResolutionScale:(CGFloat)resolutionScale {
    _resolutionScale = resolutionScale;
    OELogToFile(@"[Libretro] Resolution scale set to: %f", resolutionScale);
}

- (void)setGamma:(CGFloat)gamma {
    _gamma = gamma;
    OELogToFile(@"[Libretro] Gamma set to: %f", gamma);
}

- (void)setSaturation:(CGFloat)saturation {
    _saturation = saturation;
    OELogToFile(@"[Libretro] Saturation set to: %f", saturation);
}

@end
