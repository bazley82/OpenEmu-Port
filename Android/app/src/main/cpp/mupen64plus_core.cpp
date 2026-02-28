/**
 * mupen64plus_core.cpp
 * JNI stub for Mupen64Plus Nintendo 64 emulator.
 *
 * The full Mupen64Plus runtime requires its plugin ecosystem (video, audio,
 * RSP, input) and external libraries (xxhash, md5) that are not present in this
 * repository. This stub provides the JNI interface so the .so is present and
 * loadable — full N64 emulation support will be wired in a future beta once
 * the plugin and dependency chain is bundled.
 */

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <string>

#define LOG_TAG "Mupen64PlusCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

static ANativeWindow *g_window = nullptr;

extern "C" {

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeLoadROM(
    JNIEnv *env, jobject /*thiz*/, jstring jpath, jstring /*jcore*/) {
  const char *path = env->GetStringUTFChars(jpath, nullptr);
  LOGW("Mupen64Plus: ROM '%s' queued — full N64 runtime (plugin ecosystem) "
       "pending Beta 8.",
       path);
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
  LOGI("Mupen64Plus: surface %s", surface ? "acquired" : "released");
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeSetSize(
    JNIEnv * /*env*/, jobject /*thiz*/, jint w, jint h) {
  LOGI("Mupen64Plus: surface size %dx%d", w, h);
}

JNIEXPORT void JNICALL Java_org_openemu_android_MainActivity_nativeSendInput(
    JNIEnv *env, jobject /*thiz*/, jstring jbutton, jboolean pressed) {
  const char *btn = env->GetStringUTFChars(jbutton, nullptr);
  LOGI("Mupen64Plus: input stub — %s %s", btn, pressed ? "DOWN" : "UP");
  env->ReleaseStringUTFChars(jbutton, btn);
}

} // extern "C"
