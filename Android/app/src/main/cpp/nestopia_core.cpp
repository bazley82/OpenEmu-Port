/**
 * nestopia_core.cpp
 * Real C++ JNI bridge for Nestopia NES emulator.
 * Execute() is a method of Nes::Api::Emulator directly (not Machine).
 */

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <atomic>
#include <cstring>
#include <fstream>
#include <jni.h>
#include <string>
#include <thread>

// Nestopia public API
#include "api/NstApiEmulator.hpp"
#include "api/NstApiInput.hpp"
#include "api/NstApiMachine.hpp"
#include "api/NstApiSound.hpp"
#include "api/NstApiVideo.hpp"

#define LOG_TAG "NestopiaCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ─────────────────────────────────────────────────────────────────────────────
// Emulator state
// ─────────────────────────────────────────────────────────────────────────────

static Nes::Api::Emulator *g_emu = nullptr;
static ANativeWindow *g_window = nullptr;
static std::atomic<bool> g_running{false};

static const int NES_WIDTH = 256;
static const int NES_HEIGHT = 240;

static uint32_t g_videoBuf[NES_WIDTH * NES_HEIGHT];
static std::atomic<unsigned> g_joypad1{0};

// ─────────────────────────────────────────────────────────────────────────────
// Emulation loop — Execute() is on Nes::Api::Emulator directly
// ─────────────────────────────────────────────────────────────────────────────

static void emulationLoop() {
  LOGI("Nestopia emulation thread started.");

  Nes::Core::Video::Output videoOut;
  videoOut.pixels = g_videoBuf;
  videoOut.pitch = NES_WIDTH * sizeof(uint32_t);

  Nes::Core::Input::Controllers controllers;

  while (g_running) {
    controllers.pad[0].buttons = g_joypad1.load();

    // Execute one NES frame (method lives on Emulator, not Machine)
    g_emu->Execute(&videoOut, nullptr, &controllers);

    if (g_window) {
      ANativeWindow_setBuffersGeometry(g_window, NES_WIDTH, NES_HEIGHT,
                                       WINDOW_FORMAT_RGBX_8888);
      ANativeWindow_Buffer buf;
      if (ANativeWindow_lock(g_window, &buf, nullptr) == 0) {
        auto *dst = reinterpret_cast<uint32_t *>(buf.bits);
        for (int y = 0; y < NES_HEIGHT; ++y)
          std::memcpy(dst + y * buf.stride, g_videoBuf + y * NES_WIDTH,
                      NES_WIDTH * sizeof(uint32_t));
        ANativeWindow_unlockAndPost(g_window);
      }
    }
  }
  LOGI("Nestopia emulation thread stopped.");
}

// ─────────────────────────────────────────────────────────────────────────────
// JNI exports
// ─────────────────────────────────────────────────────────────────────────────

extern "C" {

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeLoadROM(
    JNIEnv *env, jobject /*thiz*/, jstring jpath, jstring /*jcore*/) {
  const char *path = env->GetStringUTFChars(jpath, nullptr);

  if (!g_emu)
    g_emu = new Nes::Api::Emulator();

  std::ifstream romStream(path, std::ios::binary);
  if (!romStream.is_open()) {
    LOGE("Nestopia: cannot open ROM '%s'", path);
    env->ReleaseStringUTFChars(jpath, path);
    return;
  }

  Nes::Api::Machine machine(*g_emu);
  Nes::Result result =
      machine.Load(romStream, Nes::Api::Machine::FAVORED_NES_NTSC);
  if (NES_FAILED(result)) {
    LOGE("Nestopia: failed to load ROM '%s' (code %d)", path, result);
  } else {
    LOGI("Nestopia: loaded ROM '%s'", path);
    machine.Power(true);
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
  }
  LOGI("Nestopia: surface %s", surface ? "acquired" : "released");
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeSetSize(
    JNIEnv * /*env*/, jobject /*thiz*/, jint w, jint h) {
  LOGI("Nestopia: surface size %dx%d", w, h);
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeSendInput(
    JNIEnv *env, jobject /*thiz*/, jstring jbutton, jboolean pressed) {
  const char *btn = env->GetStringUTFChars(jbutton, nullptr);
  std::string b(btn);
  env->ReleaseStringUTFChars(jbutton, btn);

  unsigned mask = 0;
  if (b == "A")
    mask = Nes::Core::Input::Controllers::Pad::A;
  else if (b == "B")
    mask = Nes::Core::Input::Controllers::Pad::B;
  else if (b == "START")
    mask = Nes::Core::Input::Controllers::Pad::START;
  else if (b == "SELECT")
    mask = Nes::Core::Input::Controllers::Pad::SELECT;
  else if (b == "UP")
    mask = Nes::Core::Input::Controllers::Pad::UP;
  else if (b == "DOWN")
    mask = Nes::Core::Input::Controllers::Pad::DOWN;
  else if (b == "LEFT")
    mask = Nes::Core::Input::Controllers::Pad::LEFT;
  else if (b == "RIGHT")
    mask = Nes::Core::Input::Controllers::Pad::RIGHT;

  if (pressed)
    g_joypad1 |= mask;
  else
    g_joypad1 &= ~mask;
}

} // extern "C"
