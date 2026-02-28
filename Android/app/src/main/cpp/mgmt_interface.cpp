#include <android/log.h>
#include <jni.h>
#include <string>

#define LOG_TAG "OpenEmuCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring JNICALL
Java_org_openemu_android_MainActivity_stringFromJNI(JNIEnv *env,
                                                    jobject /* this */) {
  std::string hello = "OpenEmuARM64 Core Interface Initialized";
  LOGI("JNI: Core interface initialized.");
  return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_org_openemu_android_MainActivity_nativeSendInput(JNIEnv *env,
                                                      jobject /* this */,
                                                      jstring button) {
  const char *nativeButton = env->GetStringUTFChars(button, 0);
  LOGI("JNI Input: %s", nativeButton);
  // TODO: Map to Libretro/mGBA input state
  env->ReleaseStringUTFChars(button, nativeButton);
}

extern "C" JNIEXPORT void JNICALL
Java_org_openemu_android_MainActivity_nativeLoadROM(JNIEnv *env,
                                                    jobject /* this */,
                                                    jstring path) {
  const char *nativePath = env->GetStringUTFChars(path, 0);
  LOGI("JNI: Loading ROM: %s", nativePath);
  // TODO: Initialize mGBA core and load ROM
  env->ReleaseStringUTFChars(path, nativePath);
}

extern "C" JNIEXPORT void JNICALL
Java_org_openemu_android_MainActivity_nativeSetSurface(JNIEnv *env,
                                                       jobject /* this */,
                                                       jobject surface) {
  LOGI("JNI: Surface set.");
  // TODO: Initialize Vulkan/OpenGL with the native surface
}

extern "C" JNIEXPORT void JNICALL
Java_org_openemu_android_MainActivity_nativeSetSize(JNIEnv *env,
                                                    jobject /* this */,
                                                    jint width, jint height) {
  LOGI("JNI: Surface size changed: %dx%d", width, height);
}
