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

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <atomic>
#include <csignal>
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
#define RETRO_ENVIRONMENT_SET_HW_RENDER 14

enum retro_hw_context_type {
  RETRO_HW_CONTEXT_NONE = 0,
  RETRO_HW_CONTEXT_OPENGL = 1,
  RETRO_HW_CONTEXT_OPENGLES2 = 2,
  RETRO_HW_CONTEXT_OPENGL_CORE = 3,
  RETRO_HW_CONTEXT_OPENGLES3 = 4,
  RETRO_HW_CONTEXT_OPENGLES_CUSTOM = 5,
  RETRO_HW_CONTEXT_VULKAN = 6,
  RETRO_HW_CONTEXT_DUMMY = 255
};

typedef void *(*retro_hw_get_proc_address_t)(const char *);

struct retro_hw_render_callback {
  retro_hw_context_type context_type;
  void (*context_reset)();
  uintptr_t (*get_current_framebuffer)();
  void *(*get_proc_address)(const char *);
  bool depth;
  bool stencil;
  bool bottom_left_origin;
  unsigned version_major;
  unsigned version_minor;
  bool cache_context;
  void (*context_destroy)();
  bool debug_context;
};

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

// EGL Globals
static EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
static EGLContext g_eglContext = EGL_NO_CONTEXT;
static EGLSurface g_eglSurface = EGL_NO_SURFACE;
static bool g_hwRenderEnabled = false;
static retro_hw_render_callback g_hwRenderCallback = {RETRO_HW_CONTEXT_NONE};

// Beta 27: EGL context and surface are now managed exclusively in the
// emulation background thread to ensure strict thread isolation.
// All initEGL() calls from JNI threads have been removed.
static std::atomic<bool> g_running{false};
static std::atomic<bool> g_isPaused{false};
static JavaVM *g_vm = nullptr;
static std::string g_logFilePath;
static jmethodID g_logDebugMethod = nullptr;
static jmethodID g_initAudioMethod = nullptr;
static jmethodID g_writeAudioMethod = nullptr;
static jclass g_mainActivityClass = nullptr;

static void crashHandler(int sig) {
  const char *sig_name = "UNKNOWN";
  if (sig == SIGSEGV)
    sig_name = "SIGSEGV";
  else if (sig == SIGABRT)
    sig_name = "SIGABRT";
  else if (sig == SIGILL)
    sig_name = "SIGILL";

  LOGE("FATAL CRASH: Signal %d (%s)", sig, sig_name);

  // Attempt to write to crash log file
  if (!g_logFilePath.empty()) {
    FILE *f = fopen(g_logFilePath.c_str(), "a");
    if (f) {
      fprintf(f, "\nFATAL CRASH: Signal %d (%s)\n", sig, sig_name);
      fclose(f);
    }
  }

  // We can't safely call JNI here usually, but we've logged to file.
  exit(sig);
}

static void LogToHUD(const char *fmt, ...) {
  if (!g_vm || !g_logDebugMethod || !g_mainActivityClass)
    return;

  char buf[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  JNIEnv *env = nullptr;
  bool detached = false;
  if (g_vm->GetEnv((void **)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
    g_vm->AttachCurrentThread(&env, nullptr);
    detached = true;
  }

  if (env) {
    jstring jmsg = env->NewStringUTF(buf);
    env->CallStaticVoidMethod(g_mainActivityClass, g_logDebugMethod, jmsg);
    env->DeleteLocalRef(jmsg);
  }

  if (detached) {
    g_vm->DetachCurrentThread();
  }
}

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
  JNIEnv *env = nullptr;
  if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK)
    return -1;

  jclass cls = env->FindClass("org/openemu/android/MainActivity");
  g_mainActivityClass = (jclass)env->NewGlobalRef(cls);
  g_logDebugMethod = env->GetStaticMethodID(g_mainActivityClass, "logDebug",
                                            "(Ljava/lang/String;)V");
  g_initAudioMethod =
      env->GetStaticMethodID(g_mainActivityClass, "initAudio", "(I)V");
  g_writeAudioMethod =
      env->GetStaticMethodID(g_mainActivityClass, "writeAudio", "([SI)V");

  // Register signal handlers for crash logging
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = crashHandler;
  sigaction(SIGSEGV, &sa, nullptr);
  sigaction(SIGABRT, &sa, nullptr);
  sigaction(SIGILL, &sa, nullptr);

  LOGI("JNI_OnLoad called. VM: %p", (void *)vm);
  LogToFile("BC: JNI_OnLoad called. VM: %p", (void *)vm);
  LogToHUD("JNI Bridge Initialized (Beta 16/19)");

  return JNI_VERSION_1_6;
}

// Resolved core function pointers
static retro_init_t g_init = nullptr;
static retro_deinit_t g_deinit = nullptr;
static retro_get_system_info_t g_getSystemInfo = nullptr;
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

// Deferred ROM loading globals
static std::string g_pendingRomPath;
static std::vector<uint8_t> g_pendingRomData;
static bool g_pendingRomNeedFullpath = false;
static std::atomic<bool> g_loadGamePending{false};

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
#define JOYPAD_L2 12
#define JOYPAD_R2 13
#define JOYPAD_L3 14
#define JOYPAD_R3 15

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
  case RETRO_ENVIRONMENT_SET_HW_RENDER:
    if (data) {
      retro_hw_render_callback *cb =
          reinterpret_cast<retro_hw_render_callback *>(data);
      cb->context_type = RETRO_HW_CONTEXT_OPENGLES3;

      // Beta 31/32: Robust get_proc_address with dlsym fallback for Android
      cb->get_proc_address = [](const char *sym) -> void * {
        void *p = (void *)eglGetProcAddress(sym);
        if (!p) {
          // Fallback to libGLESv3.so for core functions
          static void *glesHandle = dlopen("libGLESv3.so", RTLD_LAZY);
          if (glesHandle) {
            p = dlsym(glesHandle, sym);
          }
        }
        return p;
      };

      cb->get_current_framebuffer = []() -> uintptr_t { return 0; };

      // Save a copy for our own context_reset calls later
      g_hwRenderCallback = *cb;
      g_hwRenderEnabled = true;
      LogToFile("BC: HW Rendering requested (direct population)");
      return true;
    }
    return false;
  default:
    return false;
  }
}

static void videoRefreshCallback(const void *data, unsigned width,
                                 unsigned height, size_t pitch) {
  if (g_hwRenderEnabled)
    return; // Core handles rendering to FBO/Screen via GLES

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
    static bool firstFrame = true;
    if (firstFrame) {
      LogToHUD("First frame blitted to ANativeWindow");
      firstFrame = false;
    }
    ANativeWindow_unlockAndPost(g_window);
  }
}

static size_t audioSampleBatchCallback(const int16_t *data, size_t frames) {
  if (!g_vm || !g_writeAudioMethod || !g_mainActivityClass)
    return frames;

  JNIEnv *env = nullptr;
  bool detached = false;
  if (g_vm->GetEnv((void **)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
    g_vm->AttachCurrentThread(&env, nullptr);
    detached = true;
  }

  if (env) {
    jshortArray jbuf = env->NewShortArray(frames * 2);
    env->SetShortArrayRegion(jbuf, 0, frames * 2, data);
    env->CallStaticVoidMethod(g_mainActivityClass, g_writeAudioMethod, jbuf,
                              (jint)frames);
    env->DeleteLocalRef(jbuf);
  }

  if (detached)
    g_vm->DetachCurrentThread();
  return frames;
}

static void audioSampleCallback(int16_t left, int16_t right) {
  int16_t buf[2] = {left, right};
  audioSampleBatchCallback(buf, 1);
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

  LogToHUD("Attempting dlopen: %s", soPath);
  g_coreHandle = dlopen(soPath, RTLD_LAZY | RTLD_LOCAL);
  if (!g_coreHandle) {
    const char *err = dlerror();
    LOGE("dlopen failed for '%s': %s", soPath, err ? err : "unknown error");
    LogToHUD("ERROR: dlopen failed: %s", err ? err : "unknown error");
    return false;
  }
  LogToHUD("dlopen SUCCESS");

#define RESOLVE(var, sym, type)                                                \
  var = (type)dlsym(g_coreHandle, sym);                                        \
  if (!var) {                                                                  \
    LOGE("missing symbol: %s", sym);                                           \
    LogToHUD("ERROR: Missing symbol: %s", sym);                                \
    return false;                                                              \
  }

  RESOLVE(g_init, "retro_init", retro_init_t)
  RESOLVE(g_deinit, "retro_deinit", retro_deinit_t)
  RESOLVE(g_getSystemInfo, "retro_get_system_info", retro_get_system_info_t)
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

  EGLConfig eglConfig = nullptr;
  LOGI("Thread: Pre-initializing EGL context...");
  g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (eglInitialize(g_eglDisplay, nullptr, nullptr)) {
    LogToFile("BC: Thread: eglInitialize SUCCESS");
  } else {
    LogToFile("BC: Thread: eglInitialize FAILED");
  }

  const EGLint attribs[] = {EGL_RENDERABLE_TYPE,
                            EGL_OPENGL_ES3_BIT,
                            EGL_BLUE_SIZE,
                            8,
                            EGL_GREEN_SIZE,
                            8,
                            EGL_RED_SIZE,
                            8,
                            EGL_DEPTH_SIZE,
                            24,
                            EGL_SURFACE_TYPE,
                            EGL_WINDOW_BIT,
                            EGL_NONE};
  EGLint numConfigs;
  eglChooseConfig(g_eglDisplay, attribs, &eglConfig, 1, &numConfigs);

  const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  g_eglContext =
      eglCreateContext(g_eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
  if (g_eglContext != EGL_NO_CONTEXT) {
    LogToFile("BC: Thread: eglCreateContext SUCCESS");
  }

  {
    std::lock_guard<std::mutex> lock(g_windowMutex);
    if (g_window) {
      g_eglSurface =
          eglCreateWindowSurface(g_eglDisplay, eglConfig, g_window, nullptr);
    }
  }

  if (g_eglSurface != EGL_NO_SURFACE) {
    if (eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface,
                       g_eglContext)) {
      LOGI("Thread: EGL Context Current");
      LogToFile("BC: Thread: EGL Context Current");
    } else {
      LOGE("Thread: eglMakeCurrent FAILED: 0x%x", eglGetError());
      LogToFile("BC: Thread: eglMakeCurrent FAILED");
    }
  }

  // Beta 25: Deferred initialization and game loading on the emulation thread
  if (g_loadGamePending.load()) {
    // Beta 31: Total Thread Migration
    // All setup calls MUST happen on the thread that owns the EGL context.
    LogToFile("BC: Thread: Step 2: Setting Environment...");
    if (g_setEnv)
      g_setEnv(&environmentCallback);

    LogToFile("BC: Thread: Step 3: Setting Video...");
    if (g_setVideo)
      g_setVideo(&videoRefreshCallback);

    LogToFile("BC: Thread: Step 4: Setting Audio...");
    if (g_setAudio)
      g_setAudio(&audioSampleCallback);
    if (g_setAudioBatch)
      g_setAudioBatch(&audioSampleBatchCallback);

    LogToFile("BC: Thread: Step 5: Setting Input...");
    if (g_setInputPoll)
      g_setInputPoll(&inputPollCallback);
    if (g_setInputState)
      g_setInputState(&inputStateCallback);

    // 2. Core initialization (Init requires context for some cores)
    if (g_init) {
      LOGI("Thread: retro_init called");
      LogToFile("BC: Thread: retro_init called");
      g_init();
    }

    // 3. Load Game
    if (g_loadGame && !g_pendingRomPath.empty()) {
      retro_game_info info = {0};
      info.path = g_pendingRomPath.c_str();

      if (g_pendingRomNeedFullpath) {
        info.data = nullptr;
        info.size = 0;
      } else {
        info.data = g_pendingRomData.data();
        info.size = g_pendingRomData.size();
      }

      LOGI("Thread: retro_load_game called");
      LogToFile("BC: Thread: retro_load_game called");
      if (!g_loadGame(&info)) {
        LOGE("retro_load_game failed");
        LogToHUD("ERROR: retro_load_game failed");
        g_running = false;
      } else {
        LogToHUD("retro_load_game SUCCESS");

        // 4. Context Reset (If core requested HW render during load)
        if (g_hwRenderEnabled && g_hwRenderCallback.context_reset) {
          LOGI("Thread: hw_context_reset called");
          LogToFile("BC: Thread: hw_context_reset called");
          g_hwRenderCallback.context_reset();
        }

        // Beta 22: Initialize AudioTrack with actual rate after load
        retro_system_av_info avInfo = {0};
        if (g_getAVInfo) {
          g_getAVInfo(&avInfo);
          env->CallStaticVoidMethod(g_mainActivityClass, g_initAudioMethod,
                                    (jint)avInfo.timing.sample_rate);

          std::lock_guard<std::mutex> lock(g_windowMutex);
          if (g_window) {
            ANativeWindow_setBuffersGeometry(
                g_window, (int)avInfo.geometry.base_width,
                (int)avInfo.geometry.base_height, WINDOW_FORMAT_RGBX_8888);
          }
        }
      }
    }
    g_loadGamePending.store(false);
  }

  ANativeWindow *lastWindow = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_windowMutex);
    lastWindow = g_window;
  }

  while (g_running) {
    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    if (g_run && !g_isPaused) {
      if (g_hwRenderEnabled) {
        std::lock_guard<std::mutex> lock(g_windowMutex);
        if (g_window != lastWindow) {
          LOGI("Window changed, updating EGL surface...");
          if (g_eglSurface != EGL_NO_SURFACE) {
            eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                           EGL_NO_CONTEXT);
            eglDestroySurface(g_eglDisplay, g_eglSurface);
            g_eglSurface = EGL_NO_SURFACE;
          }
          if (g_window) {
            g_eglSurface = eglCreateWindowSurface(g_eglDisplay, eglConfig,
                                                  g_window, nullptr);
          }
          lastWindow = g_window;
        }

        if (g_eglSurface != EGL_NO_SURFACE) {
          if (!eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface,
                              g_eglContext)) {
            LOGE("eglMakeCurrent failed during Run: 0x%x", eglGetError());
          }
        }
      }

      g_run();

      if (g_hwRenderEnabled && !g_isPaused && g_eglSurface != EGL_NO_SURFACE)
        eglSwapBuffers(g_eglDisplay, g_eglSurface);
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    long elapsed = (ts_end.tv_sec - ts_start.tv_sec) * 1000000000L +
                   (ts_end.tv_nsec - ts_start.tv_nsec);
    long remaining = frameNs - elapsed;

    // Use a shorter sleep when paused to keep the thread alive but responsive
    if (g_isPaused) {
      remaining = 20000000L; // 20ms
    }

    if (remaining > 0) {
      struct timespec sleep_ts = {0, remaining};
      nanosleep(&sleep_ts, nullptr);
    }
  }

  // Cleanup on exit
  if (g_unloadGame)
    g_unloadGame();
  if (g_deinit)
    g_deinit();

  if (g_hwRenderEnabled) {
    eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);
    if (g_eglSurface != EGL_NO_SURFACE)
      eglDestroySurface(g_eglDisplay, g_eglSurface);
    if (g_eglContext != EGL_NO_CONTEXT)
      eglDestroyContext(g_eglDisplay, g_eglContext);
    eglTerminate(g_eglDisplay);
    g_eglSurface = EGL_NO_SURFACE;
    g_eglContext = EGL_NO_CONTEXT;
    g_eglDisplay = EGL_NO_DISPLAY;
  }

  g_vm->DetachCurrentThread();
  LOGI("Emulation loop stopped and core deinitialized.");
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
    fprintf(f, "--- OpenEmuARM64 Beta 32 Public Logger Initialized ---\n");
    fclose(f);
  }
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativePause(
    JNIEnv *env, jobject /*thiz*/) {
  g_isPaused = true;
  LOGI("Native Emulation Paused");
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeResume(
    JNIEnv *env, jobject /*thiz*/) {
  g_isPaused = false;
  LOGI("Native Emulation Resumed");
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeStop(
    JNIEnv *env, jobject /*thiz*/) {
  g_running = false;
  g_isPaused = false;
  LOGI("Native Emulation Termination Requested");
}

/**
 * nativeLoadROM(path, coreSoPath)
 *   path      — absolute path to the cached ROM file (never a content:// URI)
 *   coreSoPath — absolute path to the core .so in the app's native lib dir
 */
JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeLoadROM(
    JNIEnv *env, jobject /*thiz*/, jstring jRomPath, jstring jCoreSoPath) {

  LogToHUD("ROM Path Resolved. Initiating boot...");
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
  LogToHUD("C++ Engine Loaded: %s", soPath.c_str());
  if (!loadCoreSO(soPath.c_str())) {
    LogToFile("BC: Step 1 FAILED.");
    LogToHUD("ERROR: Engine Load Failed.");
    LOGE("Failed to load core SO, aborting.");
    return;
  }
  LogToFile("BC: Step 1 SUCCESS. Core Handle: %p", g_coreHandle);

  // Beta 31: Callback registration moved to emulation thread.
  LogToFile("BC: Step 2: Paths resolved. Proceeding to background thread.");

  // Beta 25: Respect need_fullpath and defer loading to emulation thread
  retro_system_info sysInfo = {0};
  if (g_getSystemInfo) {
    g_getSystemInfo(&sysInfo);
    g_pendingRomNeedFullpath = sysInfo.need_fullpath;
    LogToHUD("Core Info: need_fullpath = %s",
             g_pendingRomNeedFullpath ? "true" : "false");
    LOGI("Core Info: library_name=%s, version=%s, need_fullpath=%d",
         sysInfo.library_name, sysInfo.library_version, sysInfo.need_fullpath);
  }

  g_pendingRomPath = romPath;
  g_pendingRomData.clear();

  if (!g_pendingRomNeedFullpath) {
    LogToHUD("Reading ROM into memory...");
    FILE *f = fopen(romPath.c_str(), "rb");
    if (f) {
      fseek(f, 0, SEEK_END);
      size_t fileSize = ftell(f);
      fseek(f, 0, SEEK_SET);
      g_pendingRomData.resize(fileSize);
      fread(g_pendingRomData.data(), 1, fileSize, f);
      fclose(f);
      LogToHUD("ROM Loaded: %zu bytes", g_pendingRomData.size());
    } else {
      LogToHUD("ERROR: Failed to open ROM");
      return;
    }
  }

  // Query AV info for target FPS
  retro_system_av_info avInfo{};
  if (g_getAVInfo)
    g_getAVInfo(&avInfo);
  double fps = avInfo.timing.fps > 0.0 ? avInfo.timing.fps : 60.0;

  // Start emulation loop
  LogToFile("BC: Step 10: Launching Emulation Thread...");
  LogToHUD("Emulation Loop Started");
  g_loadGamePending.store(true);
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
    LOGI("ANativeWindow initialized: %p", g_window);
    // Beta 27: EGL Surface recreation is now handled in the emulationLoop
    // thread by comparing current g_window with lastWindow.
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
  else if (b == "L2" || b == "Z")
    bit = 1u << JOYPAD_L2;
  else if (b == "R2")
    bit = 1u << JOYPAD_R2;
  else if (b == "L3")
    bit = 1u << JOYPAD_L3;
  else if (b == "R3")
    bit = 1u << JOYPAD_R3;

  if (pressed)
    g_inputState |= bit;
  else
    g_inputState &= ~bit;
}

} // extern "C"
