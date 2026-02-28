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
