#include <android/log.h>
#include <jni.h>
#include <string>
#include <vector>

// Core Headers (Modular Integration)
#ifdef CORE_NES
#include "NstApiEmulator.hpp"
#endif

#ifdef CORE_SNES
#include "snes9x.h"
#endif

#ifdef CORE_GBA
// mGBA headers
#endif

#define LOG_TAG "OpenEmuCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

enum CoreType { NONE, GBA, NES, SNES, PSX };
CoreType activeCore = NONE;

extern "C" JNIEXPORT void JNICALL
Java_org_openemu_android_MainActivity_nativeLoadROM(JNIEnv *env,
                                                    jobject /* this */,
                                                    jstring path,
                                                    jstring system) {
  const char *nativePath = env->GetStringUTFChars(path, 0);
  const char *nativeSystem = env->GetStringUTFChars(system, 0);

  std::string sysStr(nativeSystem);
  LOGI("JNI: Loading ROM for system: %s", nativeSystem);

  if (sysStr == "NES") {
    activeCore = NES;
    // Nes::Api::Emulator emu; // Real init would happen here
    LOGI("JNI: Nestopia Core selected.");
  } else if (sysStr == "SNES") {
    activeCore = SNES;
    // Settings.StopEmulation = false; // SNES9x init
    LOGI("JNI: SNES9x Core selected.");
  } else if (sysStr == "Game Boy Advance") {
    activeCore = GBA;
    LOGI("JNI: mGBA Core selected.");
  } else if (sysStr == "PlayStation") {
    activeCore = PSX;
    LOGI("JNI: PlayStation Core (Mednafen/PCSX) selected.");
  }

  env->ReleaseStringUTFChars(path, nativePath);
  env->ReleaseStringUTFChars(system, nativeSystem);
}

extern "C" JNIEXPORT void JNICALL
Java_org_openemu_android_MainActivity_nativeSendInput(JNIEnv *env,
                                                      jobject /* this */,
                                                      jstring button,
                                                      jboolean isPressed) {
  const char *nativeButton = env->GetStringUTFChars(button, 0);
  // Forward to activeCore
  // if (activeCore == NES) { ... }
  env->ReleaseStringUTFChars(button, nativeButton);
}

extern "C" JNIEXPORT void JNICALL
Java_org_openemu_android_MainActivity_nativeSetSurface(JNIEnv *env,
                                                       jobject /* this */,
                                                       jobject surface) {
  LOGI("JNI: Surface set for core rendering.");
}

extern "C" JNIEXPORT void JNICALL
Java_org_openemu_android_MainActivity_nativeSetSize(JNIEnv *env,
                                                    jobject /* this */,
                                                    jint width, jint height) {
  LOGI("JNI: Resolution changed to %dx%d", width, height);
}
