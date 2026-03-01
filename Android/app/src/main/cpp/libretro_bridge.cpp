/**
 * libretro_bridge.cpp
 * Unified Libretro frontend for OpenEmuARM64 (Beta 8).
 *
 * Loads any pre-built Libretro `.so` at runtime via dlopen(), implements the
 * required `retro_*` callback environment, and drives the emulation loop on a
 * dedicated background thread, blitting every frame to an ANativeWindow.
 *
 * Supported cores (arm64-v8a pre-built binaries in jniLibs):
 *   - gambatte_libretro_android.so      (Game Boy / Game Boy Color)
 *   - nestopia_libretro_android.so      (NES / Famicom)
 *   - mupen64plus_next_gles3_libretro_android.so  (Nintendo 64)
 *
 * JNI surface lifecycle is wired in Kotlin via EmulatorVideoSurface →
 *   nativeSetSurface() / nativeSetSize().
 */

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <jni.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Libretro public API — inline minimal declarations so we don't need headers
#define RETRO_API_VERSION 1

// retro_environment command IDs we handle
#define RETRO_ENVIRONMENT_GET_LOG_INTERFACE 27
#define RETRO_ENVIRONMENT_GET_CAN_DUPE 3
#define RETRO_ENVIRONMENT_SET_PIXEL_FORMAT 10
#define RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL 8

enum retro_pixel_format {
  RETRO_PIXEL_FORMAT_0RGB1555 = 0,
  RETRO_PIXEL_FORMAT_XRGB8888,
  RETRO_PIXEL_FORMAT_RGB565
};

struct retro_game_info {
  const char *path;
  const void *data;
  size_t size;
  const char *meta;
};

struct retro_system_av_info {
  struct {
    unsigned base_width, base_height, max_width, max_height;
    double aspect_ratio;
  } geometry;
  struct {
    double fps;
    double sample_rate;
  } timing;
};

struct retro_system_info {
  const char *library_name, *library_version, *valid_extensions;
  bool need_fullpath, block_extract;
};

typedef void (*retro_set_environment_t)(bool (*)(unsigned, void *));
typedef void (*retro_set_video_refresh_t)(void (*)(const void *, unsigned,
                                                   unsigned, size_t));
typedef void (*retro_set_audio_sample_t)(void (*)(int16_t, int16_t));
typedef void (*retro_set_audio_sample_batch_t)(size_t (*)(const int16_t *,
                                                          size_t));
typedef void (*retro_set_input_poll_t)(void (*)());
typedef void (*retro_set_input_state_t)(int16_t (*)(unsigned, unsigned,
                                                    unsigned, unsigned));
typedef void (*retro_init_t)();
typedef void (*retro_deinit_t)();
typedef unsigned (*retro_api_version_t)();
typedef void (*retro_get_system_info_t)(retro_system_info *);
typedef void (*retro_get_system_av_info_t)(retro_system_av_info *);
typedef bool (*retro_load_game_t)(const retro_game_info *);
typedef void (*retro_unload_game_t)();
typedef void (*retro_run_t)();
typedef void (*retro_reset_t)();

// ─────────────────────────────────────────────────────────────────────────────
// Bridge globals
// ─────────────────────────────────────────────────────────────────────────────

#define LOG_TAG "LibretroBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static void *g_coreHandle = nullptr;
static ANativeWindow *g_window = nullptr;
static std::mutex g_windowMutex;
static std::atomic<bool> g_running{false};
static JavaVM *g_vm = nullptr;
static std::string g_logFilePath;

static void LogToFile(const char *fmt, ...) {
  if (g_logFilePath.empty())
    return;
  FILE *f = fopen(g_logFilePath.c_str(), "a");
  if (!f)
    return;

  va_list args;
  va_start(args, fmt);
  vfprintf(f, fmt, args);
  fprintf(f, "\n");
  va_end(args);

  fflush(f);
  fclose(f);
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  g_vm = vm;
  LogToFile("BC: JNI_OnLoad called. VM: %p", (void *)vm);
  return JNI_VERSION_1_6;
}

// Resolved core function pointers
static retro_init_t g_init = nullptr;
static retro_deinit_t g_deinit = nullptr;
static retro_load_game_t g_loadGame = nullptr;
static retro_unload_game_t g_unloadGame = nullptr;
static retro_run_t g_run = nullptr;
static retro_get_system_av_info_t g_getAVInfo = nullptr;
static retro_set_environment_t g_setEnv = nullptr;
static retro_set_video_refresh_t g_setVideo = nullptr;
static retro_set_audio_sample_t g_setAudio = nullptr;
static retro_set_audio_sample_batch_t g_setAudioBatch = nullptr;
static retro_set_input_poll_t g_setInputPoll = nullptr;
static retro_set_input_state_t g_setInputState = nullptr;

// Video frame staging buffer
static std::vector<uint8_t> g_frameBuf;
static unsigned g_frameWidth = 0;
static unsigned g_frameHeight = 0;
static size_t g_framePitch = 0;
static retro_pixel_format g_pixelFmt = RETRO_PIXEL_FORMAT_XRGB8888;

// Input state (joypad port 0)
static std::atomic<uint32_t> g_inputState{0};
// Libretro joypad button IDs
#define JOYPAD_B 0
#define JOYPAD_Y 1
#define JOYPAD_SELECT 2
#define JOYPAD_START 3
#define JOYPAD_UP 4
#define JOYPAD_DOWN 5
#define JOYPAD_LEFT 6
#define JOYPAD_RIGHT 7
#define JOYPAD_A 8
#define JOYPAD_X 9
#define JOYPAD_L 10
#define JOYPAD_R 11

// ─────────────────────────────────────────────────────────────────────────────
// Libretro callback implementations
// ─────────────────────────────────────────────────────────────────────────────

static bool environmentCallback(unsigned cmd, void *data) {
  switch (cmd) {
  case RETRO_ENVIRONMENT_GET_CAN_DUPE:
    if (data)
      *reinterpret_cast<bool *>(data) = true;
    return true;
  case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
    if (data)
      g_pixelFmt = *reinterpret_cast<retro_pixel_format *>(data);
    return true;
  case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
    return true;
  default:
    return false;
  }
}

static void videoRefreshCallback(const void *data, unsigned width,
                                 unsigned height, size_t pitch) {
  if (!data)
    return; // duplicate frame signal

  g_frameWidth = width;
  g_frameHeight = height;

  // Blit to ANativeWindow
  std::lock_guard<std::mutex> lock(g_windowMutex);
  if (!g_window)
    return;

  ANativeWindow_Buffer buf;
  if (ANativeWindow_lock(g_window, &buf, nullptr) == 0) {
    const auto *src8 = reinterpret_cast<const uint8_t *>(data);
    auto *dst8 = reinterpret_cast<uint8_t *>(buf.bits);

    for (unsigned y = 0; y < height; ++y) {
      if (g_pixelFmt == RETRO_PIXEL_FORMAT_RGB565) {
        // Convert RGB565 → RGBX8888 (Android format)
        const auto *row = reinterpret_cast<const uint16_t *>(src8 + y * pitch);
        auto *dst = reinterpret_cast<uint32_t *>(dst8 + y * buf.stride * 4);
        for (unsigned x = 0; x < width; ++x) {
          uint16_t p = row[x];
          uint8_t r = ((p >> 11) & 0x1F) << 3;
          uint8_t g = ((p >> 5) & 0x3F) << 2;
          uint8_t b = (p & 0x1F) << 3;
          dst[x] = (r << 0) | (g << 8) | (b << 16) | (0xFFu << 24);
        }
      } else {
        // Assume XRGB8888 or similar — blit line by line respecting stride
        std::memcpy(dst8 + y * buf.stride * 4, src8 + y * pitch, width * 4);
      }
    }
    ANativeWindow_unlockAndPost(g_window);
  }
}

static void audioSampleCallback(int16_t /*left*/, int16_t /*right*/) {}

static size_t audioSampleBatchCallback(const int16_t * /*data*/,
                                       size_t frames) {
  return frames; // discard audio for now — Beta 9 wires AudioTrack
}

static void inputPollCallback() {}

static int16_t inputStateCallback(unsigned port, unsigned /*device*/,
                                  unsigned /*index*/, unsigned id) {
  if (port != 0)
    return 0;
  uint32_t state = g_inputState.load();
  return (state & (1u << id)) ? 1 : 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Core lifecycle helpers
// ─────────────────────────────────────────────────────────────────────────────

static bool loadCoreSO(const char *soPath) {
  if (g_coreHandle) {
    dlclose(g_coreHandle);
    g_coreHandle = nullptr;
  }

  g_coreHandle = dlopen(soPath, RTLD_LAZY | RTLD_LOCAL);
  if (!g_coreHandle) {
    LOGE("dlopen failed for '%s': %s", soPath, dlerror());
    return false;
  }

#define RESOLVE(var, sym, type)                                                \
  var = (type)dlsym(g_coreHandle, sym);                                        \
  if (!var) {                                                                  \
    LOGE("missing symbol: %s", sym);                                           \
    return false;                                                              \
  }

  RESOLVE(g_init, "retro_init", retro_init_t)
  RESOLVE(g_deinit, "retro_deinit", retro_deinit_t)
  RESOLVE(g_loadGame, "retro_load_game", retro_load_game_t)
  RESOLVE(g_unloadGame, "retro_unload_game", retro_unload_game_t)
  RESOLVE(g_run, "retro_run", retro_run_t)
  RESOLVE(g_getAVInfo, "retro_get_system_av_info", retro_get_system_av_info_t)
  RESOLVE(g_setEnv, "retro_set_environment", retro_set_environment_t)
  RESOLVE(g_setVideo, "retro_set_video_refresh", retro_set_video_refresh_t)
  RESOLVE(g_setAudio, "retro_set_audio_sample", retro_set_audio_sample_t)
  RESOLVE(g_setAudioBatch, "retro_set_audio_sample_batch",
          retro_set_audio_sample_batch_t)
  RESOLVE(g_setInputPoll, "retro_set_input_poll", retro_set_input_poll_t)
  RESOLVE(g_setInputState, "retro_set_input_state", retro_set_input_state_t)
#undef RESOLVE

  LOGI("Core loaded: %s", soPath);
  return true;
}

static void emulationLoop(double targetFps) {
  LOGI("Emulation loop started at %.2f fps", targetFps);
  const long frameNs = (long)(1e9 / targetFps);

  JNIEnv *env = nullptr;
  if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
    LOGE("Failed to attach emulation thread to JVM");
    return;
  }

  while (g_running) {
    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    if (g_run)
      g_run();

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    long elapsed = (ts_end.tv_sec - ts_start.tv_sec) * 1000000000L +
                   (ts_end.tv_nsec - ts_start.tv_nsec);
    long remaining = frameNs - elapsed;
    if (remaining > 0) {
      struct timespec sleep_ts = {0, remaining};
      nanosleep(&sleep_ts, nullptr);
    }
  }

  g_vm->DetachCurrentThread();
  LOGI("Emulation loop stopped.");
}

// ─────────────────────────────────────────────────────────────────────────────
// JNI Exports
// ─────────────────────────────────────────────────────────────────────────────

extern "C" {

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeInitLogger(
    JNIEnv *env, jobject /*thiz*/, jstring jLogDir) {
  const char *logDir = env->GetStringUTFChars(jLogDir, nullptr);
  g_logFilePath = std::string(logDir) + "/openemu_crash_log.txt";
  env->ReleaseStringUTFChars(jLogDir, logDir);

  // Clear the log for a new session
  FILE *f = fopen(g_logFilePath.c_str(), "w");
  if (f) {
    fprintf(f, "--- OpenEmuARM64 Beta 14 Logger Initialized ---\n");
    fclose(f);
  }
}

/**
 * nativeLoadROM(path, coreSoPath)
 *   path      — absolute path to the cached ROM file (never a content:// URI)
 *   coreSoPath — absolute path to the core .so in the app's native lib dir
 */
JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeLoadROM(
    JNIEnv *env, jobject /*thiz*/, jstring jRomPath, jstring jCoreSoPath) {

  LogToFile("BC: nativeLoadROM entered. Window: %p", (void *)g_window);

  // Beta 13: Copy paths to local std::string and release JNI pointers early
  const char *romPathInternal = env->GetStringUTFChars(jRomPath, nullptr);
  const char *soPathInternal = env->GetStringUTFChars(jCoreSoPath, nullptr);
  std::string romPath(romPathInternal);
  std::string soPath(soPathInternal);
  env->ReleaseStringUTFChars(jRomPath, romPathInternal);
  env->ReleaseStringUTFChars(jCoreSoPath, soPathInternal);

  LogToFile("BC: Strings copied. ROM: %s, SO: %s", romPath.c_str(),
            soPath.c_str());

  // Stop any running emulation
  g_running = false;

  LOGI("Loading core SO: %s", soPath.c_str());
  LogToFile("BC: Step 1: Loading core SO...");
  if (!loadCoreSO(soPath.c_str())) {
    LogToFile("BC: Step 1 FAILED.");
    LOGE("Failed to load core SO, aborting.");
    return;
  }
  LogToFile("BC: Step 1 SUCCESS. Core Handle: %p", g_coreHandle);

  // Beta 13: Strict Libretro Boot Sequence (Callbacks -> Init -> Load)
  LogToFile("BC: Step 2: Setting Environment...");
  if (g_setEnv)
    g_setEnv(&environmentCallback);
  LogToFile("BC: Step 3: Setting Video...");
  if (g_setVideo)
    g_setVideo(&videoRefreshCallback);
  LogToFile("BC: Step 4: Setting Audio...");
  if (g_setAudio)
    g_setAudio(&audioSampleCallback);
  if (g_setAudioBatch)
    g_setAudioBatch(&audioSampleBatchCallback);
  LogToFile("BC: Step 5: Setting Input...");
  if (g_setInputPoll)
    g_setInputPoll(&inputPollCallback);
  if (g_setInputState)
    g_setInputState(&inputStateCallback);

  LogToFile("BC: Step 6: Initializing core (retro_init)... Addr: %p",
            (void *)g_init);
  LOGI("Initializing core...");
  if (g_init)
    g_init();
  LogToFile("BC: Step 6 SUCCESS.");

  // Load the ROM
  retro_game_info gameInfo{};
  gameInfo.path = romPath.c_str();
  LOGI("Loading ROM: %s", gameInfo.path);
  LogToFile("BC: Step 7: retro_load_game... Path: %s, Struct Addr: %p",
            gameInfo.path, (void *)&gameInfo);

  if (!g_loadGame || !g_loadGame(&gameInfo)) {
    LogToFile("BC: Step 7 FAILED.");
    LOGE("retro_load_game() failed for '%s'", gameInfo.path);
    if (g_deinit)
      g_deinit();
    return;
  }
  LogToFile("BC: Step 7 SUCCESS.");

  LOGI("ROM loaded successfully: %s", romPath.c_str());

  // Query AV info for frame rate and resolution
  LogToFile("BC: Step 8: retro_get_system_av_info...");
  retro_system_av_info avInfo{};
  if (g_getAVInfo)
    g_getAVInfo(&avInfo);
  LogToFile("BC: Step 8 SUCCESS. Geometry: %dx%d, FPS: %.2f",
            avInfo.geometry.base_width, avInfo.geometry.base_height,
            avInfo.timing.fps);

  double fps = avInfo.timing.fps > 0.0 ? avInfo.timing.fps : 60.0;

  // Beta 12 Fix Refined: Set buffer geometry ONCE after loading
  LogToFile("BC: Step 9: Setting Buffer Geometry...");
  {
    std::lock_guard<std::mutex> lock(g_windowMutex);
    if (g_window) {
      ANativeWindow_setBuffersGeometry(
          g_window, (int)avInfo.geometry.base_width,
          (int)avInfo.geometry.base_height, WINDOW_FORMAT_RGBX_8888);
      LOGI("Native buffer geometry set: %dx%d", (int)avInfo.geometry.base_width,
           (int)avInfo.geometry.base_height);
    } else {
      LogToFile("BC: Step 9 WARNING: g_window is NULL");
    }
  }
  LogToFile("BC: Step 9 SUCCESS.");

  // Start emulation loop
  LogToFile("BC: Step 10: Launching Emulation Thread...");
  g_running = true;
  std::thread(emulationLoop, fps).detach();
  LogToFile("BC: Step 10 SUCCESS. nativeLoadROM exit.");
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeSetSurface(
    JNIEnv *env, jobject /*thiz*/, jobject surface) {
  std::lock_guard<std::mutex> lock(g_windowMutex);
  if (g_window) {
    ANativeWindow_release(g_window);
    g_window = nullptr;
  }
  if (surface) {
    g_window = ANativeWindow_fromSurface(env, surface);
    LOGI("Surface acquired: %p", (void *)g_window);
  } else {
    LOGI("Surface released.");
  }
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeSetSize(
    JNIEnv * /*env*/, jobject /*thiz*/, jint w, jint h) {
  LOGI("Surface resized: %dx%d", w, h);
  // ANativeWindow geometry is updated per-frame in videoRefreshCallback
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeSendInput(
    JNIEnv *env, jobject /*thiz*/, jstring jbutton, jboolean pressed) {
  const char *btn = env->GetStringUTFChars(jbutton, nullptr);
  std::string b(btn);
  env->ReleaseStringUTFChars(jbutton, btn);

  uint32_t bit = 0;
  if (b == "A")
    bit = 1u << JOYPAD_A;
  else if (b == "B")
    bit = 1u << JOYPAD_B;
  else if (b == "X")
    bit = 1u << JOYPAD_X;
  else if (b == "Y")
    bit = 1u << JOYPAD_Y;
  else if (b == "START")
    bit = 1u << JOYPAD_START;
  else if (b == "SELECT")
    bit = 1u << JOYPAD_SELECT;
  else if (b == "UP")
    bit = 1u << JOYPAD_UP;
  else if (b == "DOWN")
    bit = 1u << JOYPAD_DOWN;
  else if (b == "LEFT")
    bit = 1u << JOYPAD_LEFT;
  else if (b == "RIGHT")
    bit = 1u << JOYPAD_RIGHT;
  else if (b == "L")
    bit = 1u << JOYPAD_L;
  else if (b == "R")
    bit = 1u << JOYPAD_R;

  if (pressed)
    g_inputState |= bit;
  else
    g_inputState &= ~bit;
}

} // extern "C"
