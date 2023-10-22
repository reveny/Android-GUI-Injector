#ifndef DAWN_LOGGER_H
#define DAWN_LOGGER_H

#include <android/log.h>
#include <jni.h>
#include <cstdio>
#include <cstdlib>

#define TAG "RevenyInjector"
enum daLogType {
    daDEBUG = 3,
    daERROR = 6,
    daINFO = 4,
    daWARN = 5
};

inline void CustomLog(daLogType type, const char* tag, const char* message, ...) {
    char msg[256];
    va_list arg;
    va_start(arg, message);
    vsnprintf(msg, 120, message, arg);

    // Print to logcat
    (void)__android_log_print(type, TAG, msg);

    // Pass to java
    /*
    if (log_env != nullptr) {
        jclass log_mgr = log_env->FindClass("com/reveny/injector/v2/LogManager");
        jmethodID add_log = log_env->GetStaticMethodID(log_mgr, "AddLog", "(Ljava/lang/String;)V");

        // Convert log to string and call
        jstring str = log_env->NewStringUTF(msg);
        log_env->CallStaticVoidMethod(log_mgr, add_log, str);
    }
     */

    // TODO: Fix pass log to java or write log to a file
}

#define LOGD(...) ((void)CustomLog(daDEBUG, TAG, __VA_ARGS__))
#define LOGE(...) ((void)CustomLog(daERROR, TAG, __VA_ARGS__))
#define LOGI(...) ((void)CustomLog(daINFO,  TAG, __VA_ARGS__))
#define LOGW(...) ((void)CustomLog(daWARN,  TAG, __VA_ARGS__))

#endif