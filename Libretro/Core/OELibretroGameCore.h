#import <OpenEmuBase/OEGameCore.h>
#import "libretro.h"

#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>

NS_ASSUME_NONNULL_BEGIN

@interface OELibretroGameCore : OEGameCore
{
    @public
    BOOL _isPSP;
    BOOL _isN64;
    BOOL _isPSX;
    BOOL _isSNES;
    BOOL _isGenesis;
    BOOL _isNDS;
    void *_coreHandle;
    
    // Libretro function pointers
    void (*_retro_init)(void);
    void (*_retro_deinit)(void);
    unsigned (*_retro_api_version)(void);
    void (*_retro_get_system_info)(struct retro_system_info *info);
    void (*_retro_get_system_av_info)(struct retro_system_av_info *info);
    void (*_retro_set_controller_port_device)(unsigned port, unsigned device);
    void (*_retro_reset)(void);
    void (*_retro_run)(void);
    size_t (*_retro_serialize_size)(void);
    bool (*_retro_serialize)(void *data, size_t size);
    bool (*_retro_unserialize)(const void *data, size_t size);
    void (*_retro_cheat_reset)(void);
    void (*_retro_cheat_set)(unsigned index, bool enabled, const char *code);
    bool (*_retro_load_game)(const struct retro_game_info *game);
    bool (*_retro_load_game_special)(unsigned game_type, const struct retro_game_info *info, size_t num_info);
    void (*_retro_unload_game)(void);
    unsigned (*_retro_get_region)(void);
    void (*_retro_get_memory_data)(unsigned id);
    size_t (*_retro_get_memory_size)(unsigned id);
    
    // Core state
    struct retro_system_av_info _avInfo;
    struct retro_system_info _systemInfo;
    enum retro_pixel_format _pixelFormat;
    
    // Buffers and scaling
    void *_videoBuffer;
    void *_rendererBuffer;
    NSUInteger _videoBufferSize;
    
    // Input state (Retropad)
    // 16 buttons per player, support up to 8 players for now
    int16_t _inputState[8][16];
    
    // Mouse/Touch state
    int _mouseX;
    int _mouseY;
    BOOL _mousePressed;
    
    // ROM data buffer for cores that require memory loading
    NSData *_gameData;
    
    // Hardware rendering
    struct retro_hw_render_callback _hwRenderCallback;
    BOOL _isHWContextActive;
    id<MTLTexture> _metalTexture;
    uint32_t _currentWidth, _currentHeight;
    int _interfaceLoopCount;
    BOOL _firstFrame;
}

@property (nonatomic, assign) CGFloat resolutionScale;
@property (nonatomic, assign) CGFloat gamma;
@property (nonatomic, assign) CGFloat saturation;

@end

NS_ASSUME_NONNULL_END
