//
// Created by reveny on 5/17/24.
//
#pragma once

#include <android/log.h>
#include <jni.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

#define TAG "RevenyInjector"
enum daLogType {
    daDEBUG = 3,
    daERROR = 6,
    daINFO = 4,
    daWARN = 5
};

inline std::vector<std::string> log_messages{};
inline void CustomLog(daLogType type, const char* tag, const char* message, ...) {
    char msg[256];
    va_list arg;
    va_start(arg, message);
    vsnprintf(msg, 256, message, arg);

    // Print to logcat
    (void)__android_log_print(type, tag, "%s", msg);

    // Add to vector so we can print later
    log_messages.push_back(msg);
}

#define LOGD(...) ((void)CustomLog(daDEBUG, TAG, __VA_ARGS__))
#define LOGE(...) ((void)CustomLog(daERROR, TAG, __VA_ARGS__))
#define LOGI(...) ((void)CustomLog(daINFO,  TAG, __VA_ARGS__))
#define LOGW(...) ((void)CustomLog(daWARN,  TAG, __VA_ARGS__))