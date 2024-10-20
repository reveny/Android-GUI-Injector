//
// Created by reveny on 5/17/24.
//
#pragma once

#include <string>
#include <jni.h>
#include <android/log.h>
#include <stdexcept>

#define CHECK_JNI_EXCEPTION(env) \
    if ((env)->ExceptionCheck()) { \
        (env)->ExceptionDescribe(); \
        (env)->ExceptionClear(); \
        throw std::runtime_error("JNI Exception occurred"); \
    }

#define GET_JAVA_STRING(env, obj, method) \
    ({ \
        std::string result; \
        if (env == nullptr || obj == nullptr) { \
            throw std::invalid_argument("JNIEnv or jobject is null in GET_JAVA_STRING"); \
        } \
        jstring jstr = (jstring)env->CallObjectMethod(obj, env->GetMethodID(env->GetObjectClass(obj), method, "()Ljava/lang/String;")); \
        CHECK_JNI_EXCEPTION(env); \
        const char* chars = env->GetStringUTFChars(jstr, nullptr); \
        if (chars != nullptr) { \
            result = chars; \
            env->ReleaseStringUTFChars(jstr, chars); \
        } \
        env->DeleteLocalRef(jstr); \
        result; \
    })

#define GET_JAVA_BOOL(env, obj, method) \
    ({ \
        jboolean result = false; \
        if (env == nullptr || obj == nullptr) { \
            throw std::invalid_argument("JNIEnv or jobject is null in GET_JAVA_BOOL"); \
        } \
        jclass cls = env->GetObjectClass(obj); \
        if (cls != nullptr) { \
            jmethodID mid = env->GetMethodID(cls, method, "()Z"); \
            CHECK_JNI_EXCEPTION(env); \
            result = env->CallBooleanMethod(obj, mid); \
            env->DeleteLocalRef(cls); \
        } else { \
            throw std::runtime_error("Failed to get class in GET_JAVA_BOOL"); \
        } \
        result; \
    })

struct RemoteInjectorData {
    uintptr_t packageName;
    uintptr_t launcherActivity;
    uintptr_t libraryPath;

    bool shouldAutoLaunch;
    bool shouldKillBeforeLaunch;
    bool injectZygote;
    bool remapLibrary;

    bool useProxy;
    bool randomizeProxyName;
    bool copyToCache;
    bool hideLibrary;

    bool bypassNamespaceRestrictions;
};

class InjectorData {
private:
    std::string packageName;
    std::string launcherActivity;
    std::string libraryPath;

    bool shouldAutoLaunch;
    bool shouldKillBeforeLaunch;
    bool injectZygote;
    bool remapLibrary;

    bool useProxy;
    bool randomizeProxyName;
    bool copyToCache;
    bool hideLibrary;

    bool bypassNamespaceRestrictions;

public:
    InjectorData() {
        this->shouldAutoLaunch = false;
        this->shouldKillBeforeLaunch = false;
        this->injectZygote = false;
        this->remapLibrary = false;
        this->useProxy = false;
        this->randomizeProxyName = false;
        this->copyToCache = false;
        this->hideLibrary = false;
        this->bypassNamespaceRestrictions = false;
    }

    InjectorData(JNIEnv* env, jobject data) {
        if (env == nullptr || data == nullptr) {
            throw std::invalid_argument("JNIEnv or jobject is null in constructor");
        }
        try {
            this->packageName = GET_JAVA_STRING(env, data, "getPackageName");
            this->launcherActivity = GET_JAVA_STRING(env, data, "getLauncherActivity");
            this->libraryPath = GET_JAVA_STRING(env, data, "getLibraryPath");

            this->shouldAutoLaunch = GET_JAVA_BOOL(env, data, "isShouldAutoLaunch");
            this->shouldKillBeforeLaunch = GET_JAVA_BOOL(env, data, "isShouldKillBeforeLaunch");
            this->injectZygote = GET_JAVA_BOOL(env, data, "isInjectZygote");
            this->remapLibrary = GET_JAVA_BOOL(env, data, "isRemapLibrary");

            this->useProxy = GET_JAVA_BOOL(env, data, "isUseProxy");
            this->randomizeProxyName = GET_JAVA_BOOL(env, data, "isRandomizeProxyName");
            this->copyToCache = GET_JAVA_BOOL(env, data, "isCopyToCache");
            this->hideLibrary = GET_JAVA_BOOL(env, data, "isHideLibrary");
            this->bypassNamespaceRestrictions = GET_JAVA_BOOL(env, data, "isBypassNamespaceRestrictions");
        } catch (const std::exception& e) {
            __android_log_print(ANDROID_LOG_ERROR, "RevenyInjector", "Error in JNI operation: %s", e.what());
            throw;
        }
    }

    void setPackageName(const std::string& _packageName) {
        if (_packageName.empty()) {
            throw std::invalid_argument("Package name cannot be empty");
        }
        this->packageName = _packageName;
    }

    void setLauncherActivity(const std::string& _launcherActivity) {
        if (_launcherActivity.empty()) {
            throw std::invalid_argument("Launcher activity cannot be empty");
        }
        this->launcherActivity = _launcherActivity;
    }

    void setLibraryPath(const std::string& _libraryPath) {
        if (_libraryPath.empty()) {
            throw std::invalid_argument("Library path cannot be empty");
        }
        this->libraryPath = _libraryPath;
    }

    void setShouldAutoLaunch(bool _shouldAutoLaunch) {
        this->shouldAutoLaunch = _shouldAutoLaunch;
    }

    void setShouldKillBeforeLaunch(bool _shouldKillBeforeLaunch) {
        this->shouldKillBeforeLaunch = _shouldKillBeforeLaunch;
    }

    void setInjectZygote(bool _injectZygote) {
        this->injectZygote = _injectZygote;
    }

    void setRemapLibrary(bool _remapLibrary) {
        this->remapLibrary = _remapLibrary;
    }

    void setUseProxy(bool _useProxy) {
        this->useProxy = _useProxy;
    }

    void setRandomizeProxyName(bool _randomizeProxyName) {
        this->randomizeProxyName = _randomizeProxyName;
    }

    void setCopyToCache(bool _copyToCache) {
        this->copyToCache = _copyToCache;
    }

    void setHideLibrary(bool _hideLibrary) {
        this->hideLibrary = _hideLibrary;
    }

    void setBypassNamespaceRestrictions(bool _bypassNamespaceRestrictions) {
        this->bypassNamespaceRestrictions = _bypassNamespaceRestrictions;
    }

    std::string getPackageName() {
        return this->packageName;
    }

    std::string getLauncherActivity() {
        return this->launcherActivity;
    }

    std::string getLibraryPath() {
        return this->libraryPath;
    }

    bool getShouldAutoLaunch() {
        return this->shouldAutoLaunch;
    }

    bool getShouldKillBeforeLaunch() {
        return this->shouldKillBeforeLaunch;
    }

    bool getInjectZygote() {
        return this->injectZygote;
    }

    bool getRemapLibrary() {
        return this->remapLibrary;
    }

    bool getUseProxy() {
        return this->useProxy;
    }

    bool getRandomizeProxyName() {
        return this->randomizeProxyName;
    }

    bool getCopyToCache() {
        return this->copyToCache;
    }

    bool getHideLibrary() {
        return this->hideLibrary;
    }

    bool getBypassNamespaceRestrictions() {
        return this->bypassNamespaceRestrictions;
    }
};