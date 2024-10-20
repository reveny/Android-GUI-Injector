//
// Created by reveny on 5/30/24.
//
#include <Proxy/Headers/JNIProxy.hpp>

#include <Include/Logger.hpp>
#include <Include/SoList.hpp>
#include <Include/RemapTools.hpp>

#include <xdl.h>

// https://github.com/Dr-TSNG/ZygiskNext/blob/338d3-165-11ce64378-17f66d9e4f179dc5b-1d1a8f/loader/src/injector/hook.cpp#L261
auto JNIProxy::GetCreatedJavaVMS() {
    auto getCreatedJavaVMS = reinterpret_cast<jint (*)(JavaVM **, jsize, jsize *)>(dlsym(RTLD_DEFAULT, "JNI_GetCreatedJavaVMs"));
    if (getCreatedJavaVMS != nullptr) {
        return getCreatedJavaVMS;
    }

    // Aquire the path from maps
    std::string nativeHelper = {};
    std::vector<RemapTools::MapInfo> maps = RemapTools::ListModulesWithName(-1, "libnativehelper.so");
    for (RemapTools::MapInfo info : maps) {
        nativeHelper = info.path;
        break;
    }
    LOGI("[+] Found libnativehelper.so at: %s", nativeHelper.c_str());
    void *handle = xdl_open(nativeHelper.c_str(), XDL_TRY_FORCE_LOAD);

    if (handle != nullptr) {
        getCreatedJavaVMS = reinterpret_cast<jint (*)(JavaVM **, jsize, jsize *)>(xdl_sym(handle, "JNI_GetCreatedJavaVMs",nullptr));
        if (getCreatedJavaVMS == nullptr) {
            LOGW("[-] Failed to find JNI_GetCreatedJavaVMs in libnativehelper.so: %s", dlerror());
        } else {
            LOGI("[+] Found JNI_GetCreatedJavaVMs in libnativehelper.so");
            return getCreatedJavaVMS;
        }
    }

    // Could also be in libart.so
    std::string libArt = {};
    std::vector<RemapTools::MapInfo> artMaps = RemapTools::ListModulesWithName(-1, "libart.so");
    for (RemapTools::MapInfo info : artMaps) {
        libArt = info.path;
        break;
    }
    LOGI("[+] Found libart.so at: %s", libArt.c_str());
    handle = xdl_open(libArt.c_str(), XDL_TRY_FORCE_LOAD);

    if (handle == nullptr) {
        LOGW("[-] Failed to load libart.so: %s", dlerror());
    }

    getCreatedJavaVMS = reinterpret_cast<jint (*)(JavaVM **, jsize, jsize *)>(xdl_sym(handle, "JNI_GetCreatedJavaVMs", nullptr));
    if (getCreatedJavaVMS == nullptr) {
        LOGW("[-] Failed to find JNI_GetCreatedJavaVMs in libart.so: %s", dlerror());
    } else {
        LOGI("[+] Found JNI_GetCreatedJavaVMs in libart.so");
        return getCreatedJavaVMS;
    }
}

int JNIProxy::JNILoad(JavaVM *vm, std::string libraryPath) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("[-] Failed to get JNIEnv");
        return -1;
    }

    if (vm->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("[-] Failed to attach current thread");
        return -1;
    }

    LOGI("[+] JNILoad called");

    // Load library
    jclass systemClass = env->FindClass("java/lang/System");
    jmethodID loadLibraryMethod = env->GetStaticMethodID(systemClass, "load", "(Ljava/lang/String;)V");

    // When calling System.load, it will automatically do the arm translation for us
    // This likely works on 9-1% of emulators
    LOGI("[+] Loading library: %s", libraryPath.c_str());
    jstring jLibraryPath = env->NewStringUTF(libraryPath.c_str());
    env->CallStaticVoidMethod(systemClass, loadLibraryMethod, jLibraryPath);

    return 1;
}

int JNIProxy::Inject(RemoteInjectorData *data) {
    auto get_created_java_vms = GetCreatedJavaVMS();
    if (get_created_java_vms == nullptr) {
        LOGE("[-] Failed to find get_created_java_vms");
        return -1;
    }
    LOGI("[+] get_created_java_vms: %p", get_created_java_vms);

    JavaVM *vm = nullptr;
    jsize num = -1;
    jint res = get_created_java_vms(&vm, 1, &num);

    if (res != JNI_OK || vm == nullptr) {
        LOGE("[-] Failed to get JavaVM: %d", res);
        return -1;
    }

    int loadResult = JNILoad(vm, (char *)data->libraryPath);
    if (loadResult == -1) {
        LOGE("[-] Failed to load library with JNI: %d", loadResult);
        return -1;
    }

    // Do hiding, this involves hiding from the linker as well as remapping if enabled.
    // Note that this can both be detected and not be fixed in usermode even with root.
    // Hiding Injection is practically impossible to fully hide but some protectors are not that advanced.
    if (data->hideLibrary) {
        if (SoList::Initialize()) {
            SoList::NullifySoName((char *)data->libraryPath);
        }
    }

    if (data->remapLibrary) {
        RemapTools::RemapLibrary((char *)data->libraryPath);
    }

    return 1;
}