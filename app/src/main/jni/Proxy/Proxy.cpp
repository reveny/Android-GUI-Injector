//
// Created by reveny on 5/17/24.
//

#include <jni.h>
#include <dlfcn.h>

#include <xdl.h>
#include <unistd.h>
#include <android/dlext.h>

#include "Include/Logger.hpp"
#include "Include/SoList.hpp"
#include "Include/RemapTools.hpp"

auto GetCreatedJavaVMS() {
    // https://github.com/Dr-TSNG/ZygiskNext/blob/338d3-165-11ce64378-17f66d9e4f179dc5b-1d1a8f/loader/src/injector/hook.cpp#L261
    auto getCreatedJavaVMS = reinterpret_cast<jint (*)(JavaVM **, jsize, jsize *)>(dlsym(RTLD_DEFAULT, "JNI_GetCreatedJavaVMs"));
    if (getCreatedJavaVMS == nullptr) {
        // Aquire the path from maps
        #if defined(__LP64__)
            std::string nativeHelper = "/apex/com.android.art/lib64/libnativehelper.so";
        #else
            std::string nativeHelper = "/apex/com.android.art/lib/libnativehelper.so";
        #endif
        std::vector<RemapTools::ProcMapInfo> maps = RemapTools::ListModulesWithName("libnativehelper.so");
        for (RemapTools::ProcMapInfo info : maps) {
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
        // TODO: Check if some emulators use a different path
        #if defined(__LP64__)
            std::string libArt = "/apex/com.android.art/lib64/libart.so";
        #else
            std::string libArt = "/apex/com.android.art/lib/libart.so";
        #endif
        std::vector<RemapTools::ProcMapInfo> artMaps = RemapTools::ListModulesWithName("libart.so");
        for (RemapTools::ProcMapInfo info : artMaps) {
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

    return getCreatedJavaVMS;
}


int JNILoad(JavaVM *vm, const char* libraryPath) {
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
    LOGI("[+] Loading library: %s", libraryPath);
    jstring jLibraryPath = env->NewStringUTF(libraryPath);
    env->CallStaticVoidMethod(systemClass, loadLibraryMethod, jLibraryPath);

    return 1;
}

extern "C" {
    __attribute__((visibility("default")))
    int LoadWithProxy(const char* libraryPath, bool remap, bool hideLibrary, bool bypassNamespaceRestrictions) {
        LOGI("[+] LoadWithProxy called");

        // TODO: We can replicate what xdl does with ptrace in the future
        if (bypassNamespaceRestrictions) {
            LOGI("[+] Bypassing namespace restrictions");

            void *handle = xdl_open(libraryPath, XDL_ALWAYS_FORCE_LOAD);
            if (handle == nullptr) {
                LOGE("[-] Failed to load library: %s", dlerror());
                return -1;
            }
            LOGI("[+] Successfully loaded library with xdl: %s [%p]", libraryPath, handle);

            return 1;
        }

        // This is not needed if bypass namespace restrictions is enabled
        // Initialize JNI. I don't think it's a good idea to use JNI_OnLoad here because
        // we still need to pass the data somehow and we can't do that with JNI_OnLoad
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

        int loadResult = JNILoad(vm, libraryPath);
        if (loadResult == -1) {
            LOGE("[-] Failed to load library with JNI: %d", loadResult);
            return -1;
        }

        // Do hiding, this involves hiding from the linker as well as remapping if enabled.
        if (hideLibrary) {
            if (SoList::Initialize()) {
                SoList::NullifySoName(libraryPath);
            }
        }

        if (remap) {
            RemapTools::RemapLibrary(libraryPath);
        }

        return 1;
    }
}