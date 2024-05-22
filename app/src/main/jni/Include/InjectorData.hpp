//
// Created by reveny on 5/17/24.
//
#pragma once

#include <string>
#include <jni.h>
#include <android/log.h>

#define GET_JAVA_STRING(env, obj, method) \
    ({ \
        jstring jstr = (jstring)env->CallObjectMethod(obj, env->GetMethodID(env->GetObjectClass(obj), method, "()Ljava/lang/String;")); \
        const char* chars = env->GetStringUTFChars(jstr, nullptr); \
        std::string result; \
        if (chars != nullptr) { \
            result = chars; \
            env->ReleaseStringUTFChars(jstr, chars); \
        } \
        env->DeleteLocalRef(jstr); \
        result; \
    })

#define GET_JAVA_BOOL(env, obj, method) \
    ({ \
        jclass cls = env->GetObjectClass(obj); \
        jmethodID mid = env->GetMethodID(cls, method, "()Z"); \
        jboolean result = env->CallBooleanMethod(obj, mid); \
        env->DeleteLocalRef(cls); \
        result; \
    })

class InjectorData {
private:
    std::string packageName;
    std::string launcherActivity;
    std::string libraryPath;

    bool shouldAutoLaunch;
    bool shouldKillBeforeLaunch;
    bool remapLibrary;

    bool useProxy;
    bool randomizeProxyName;
    bool copyToCache;
    bool hideLibrary;

    // Until I figure out how to steal the code from xdl, this requires usage of the proxy
    bool bypassNamespaceRestrictions;

public:
    InjectorData() {
        this->shouldAutoLaunch = false;
        this->shouldKillBeforeLaunch = false;
        this->remapLibrary = false;
        this->useProxy = false;
        this->randomizeProxyName = false;
        this->copyToCache = false;
        this->hideLibrary = false;
        this->bypassNamespaceRestrictions = false;
    }

    InjectorData(JNIEnv* env, jobject data) {
        this->packageName = GET_JAVA_STRING(env, data, "getPackageName");
        this->launcherActivity = GET_JAVA_STRING(env, data, "getLauncherActivity");
        this->libraryPath = GET_JAVA_STRING(env, data, "getLibraryPath");

        this->shouldAutoLaunch = GET_JAVA_BOOL(env, data, "isShouldAutoLaunch");
        this->shouldKillBeforeLaunch = GET_JAVA_BOOL(env, data, "isShouldKillBeforeLaunch");
        this->remapLibrary = GET_JAVA_BOOL(env, data, "isRemapLibrary");

        this->useProxy = GET_JAVA_BOOL(env, data, "isUseProxy");
        this->randomizeProxyName = GET_JAVA_BOOL(env, data, "isRandomizeProxyName");
        this->copyToCache = GET_JAVA_BOOL(env, data, "isCopyToCache");
        this->hideLibrary = GET_JAVA_BOOL(env, data, "isHideLibrary");
        this->bypassNamespaceRestrictions = GET_JAVA_BOOL(env, data, "isBypassNamespaceRestrictions");
    }

    void setPackageName(std::string packageName) {
        this->packageName = packageName;
    }

    void setLauncherActivity(std::string launcherActivity) {
        this->launcherActivity = launcherActivity;
    }

    void setLibraryPath(std::string libraryPath) {
        this->libraryPath = libraryPath;
    }

    void setShouldAutoLaunch(bool shouldAutoLaunch) {
        this->shouldAutoLaunch = shouldAutoLaunch;
    }

    void setShouldKillBeforeLaunch(bool shouldKillBeforeLaunch) {
        this->shouldKillBeforeLaunch = shouldKillBeforeLaunch;
    }

    void setRemapLibrary(bool remapLibrary) {
        this->remapLibrary = remapLibrary;
    }

    void setUseProxy(bool useProxy) {
        this->useProxy = useProxy;
    }

    void setRandomizeProxyName(bool randomizeProxyName) {
        this->randomizeProxyName = randomizeProxyName;
    }

    void setCopyToCache(bool copyToCache) {
        this->copyToCache = copyToCache;
    }

    void setHideLibrary(bool hideLibrary) {
        this->hideLibrary = hideLibrary;
    }

    void setBypassNamespaceRestrictions(bool bypassNamespaceRestrictions) {
        this->bypassNamespaceRestrictions = bypassNamespaceRestrictions;
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