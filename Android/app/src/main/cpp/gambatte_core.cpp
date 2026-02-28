/**
 * gambatte_core.cpp
 * Real C++ JNI bridge for the Gambatte Game Boy / Game Boy Color emulator.
 * Links against the libgambatte C++ API (gambatte::GB).
 */

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <atomic>
#include <jni.h>
#include <string>
#include <thread>
#include <vector>

// Gambatte public API
#include "libgambatte/gambatte.h"
#include "libgambatte/inputgetter.h"

#define LOG_TAG "GambatteCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ─────────────────────────────────────────────────────────────────────────────
// Input state (shared between JNI input thread and emulation thread)
// ─────────────────────────────────────────────────────────────────────────────

static std::atomic<unsigned> g_inputState{0};

class AndroidInputGetter : public gambatte::InputGetter {
public:
  unsigned operator()() override { return g_inputState.load(); }
};

// Button bit-masks matching the Gambatte/DMG standard
enum GBButton {
  GB_A = 0x01,
  GB_B = 0x02,
  GB_SELECT = 0x04,
  GB_START = 0x08,
  GB_RIGHT = 0x10,
  GB_LEFT = 0x20,
  GB_UP = 0x40,
  GB_DOWN = 0x80,
};

// ─────────────────────────────────────────────────────────────────────────────
// Emulator state
// ─────────────────────────────────────────────────────────────────────────────

static gambatte::GB *g_gb = nullptr;
static AndroidInputGetter g_inputGetter;
static std::atomic<bool> g_running{false};
static ANativeWindow *g_window = nullptr;

// Video: 160×144 pixels, RGBA8888
static const int GB_WIDTH = 160;
static const int GB_HEIGHT = 144;
static gambatte::uint_least32_t g_videoBuf[GB_WIDTH * GB_HEIGHT];

// Audio: 35112 samples/frame + 2064 slack
static const std::size_t SAMPLES_PER_FRAME = 35112;
static gambatte::uint_least32_t g_audioBuf[SAMPLES_PER_FRAME + 2064];

// ─────────────────────────────────────────────────────────────────────────────
// Emulation loop (runs on a dedicated thread)
// ─────────────────────────────────────────────────────────────────────────────

static void emulationLoop() {
  LOGI("Gambatte emulation thread started.");
  while (g_running) {
    std::size_t samples = SAMPLES_PER_FRAME;
    std::ptrdiff_t result =
        g_gb->runFor(g_videoBuf, GB_WIDTH, g_audioBuf, samples);

    if (result >= 0 && g_window) {
      // Push frame to ANativeWindow
      ANativeWindow_setBuffersGeometry(g_window, GB_WIDTH, GB_HEIGHT,
                                       WINDOW_FORMAT_RGBX_8888);
      ANativeWindow_Buffer buf;
      if (ANativeWindow_lock(g_window, &buf, nullptr) == 0) {
        auto *dst = reinterpret_cast<uint32_t *>(buf.bits);
        for (int y = 0; y < GB_HEIGHT; ++y) {
          std::memcpy(dst + y * buf.stride, g_videoBuf + y * GB_WIDTH,
                      GB_WIDTH * sizeof(uint32_t));
        }
        ANativeWindow_unlockAndPost(g_window);
      }
    }
  }
  LOGI("Gambatte emulation thread stopped.");
}

// ─────────────────────────────────────────────────────────────────────────────
// JNI exports
// ─────────────────────────────────────────────────────────────────────────────

extern "C" {

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeLoadROM(
    JNIEnv *env, jobject /*thiz*/, jstring jpath, jstring /*jcore*/) {
  const char *path = env->GetStringUTFChars(jpath, nullptr);

  if (!g_gb) {
    g_gb = new gambatte::GB();
    g_gb->setInputGetter(&g_inputGetter);
  }

  gambatte::LoadRes res = g_gb->load(std::string(path));
  if (res != gambatte::LOADRES_OK) {
    LOGE("Gambatte: failed to load ROM '%s' (code %d)", path, (int)res);
  } else {
    LOGI("Gambatte: loaded ROM '%s'", path);
    g_running = true;
    std::thread(emulationLoop).detach();
  }

  env->ReleaseStringUTFChars(jpath, path);
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeSetSurface(
    JNIEnv *env, jobject /*thiz*/, jobject surface) {
  if (g_window) {
    ANativeWindow_release(g_window);
    g_window = nullptr;
  }
  if (surface) {
    g_window = ANativeWindow_fromSurface(env, surface);
    LOGI("Gambatte: surface acquired %p", g_window);
  }
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeSetSize(
    JNIEnv * /*env*/, jobject /*thiz*/, jint w, jint h) {
  LOGI("Gambatte: surface size %dx%d", w, h);
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeSendInput(
    JNIEnv *env, jobject /*thiz*/, jstring jbutton, jboolean pressed) {
  const char *btn = env->GetStringUTFChars(jbutton, nullptr);
  std::string b(btn);
  env->ReleaseStringUTFChars(jbutton, btn);

  unsigned mask = 0;
  if (b == "A")
    mask = GB_A;
  else if (b == "B")
    mask = GB_B;
  else if (b == "START")
    mask = GB_START;
  else if (b == "SELECT")
    mask = GB_SELECT;
  else if (b == "UP")
    mask = GB_UP;
  else if (b == "DOWN")
    mask = GB_DOWN;
  else if (b == "LEFT")
    mask = GB_LEFT;
  else if (b == "RIGHT")
    mask = GB_RIGHT;

  if (pressed)
    g_inputState |= mask;
  else
    g_inputState &= ~mask;
}

} // extern "C"
