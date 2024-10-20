//
// Created by reveny on 5/17/24.
//
#include "Headers/Proxy.hpp"

#include <jni.h>
#include <dlfcn.h>

#include <xdl.h>
#include <unistd.h>
#include <android/dlext.h>

#include <Include/Logger.hpp>
#include <Include/Utility.hpp>

#include <Proxy/Headers/JNIProxy.hpp>
#include <Include/Injectordata.hpp>

extern "C" {
    EXPORT int entry(uintptr_t _data ) {
        auto *data = (RemoteInjectorData *)_data;
        LOGI("[+] Entry called from handle!");

        LOGI("[+] Package name: %s", reinterpret_cast<const char *>(data->packageName));
        LOGI("[+] Library path: %s", reinterpret_cast<const char *>(data->libraryPath));
        LOGI("[+] Launch activity: %s", reinterpret_cast<const char *>(data->launcherActivity));

        bool isZygote = Utility::GetProcessName().find("zygote") != std::string::npos;
        LOGI("[+] isZygote: %d", isZygote);

        if (isZygote) {
            LOGI("[+] Zygote process detected, doing zygote injection");

            // Zygote::Inject(data->packageName, data->libraryPath, data->remap, data->hideLibrary, data->bypassNamespaceRestrictions, data->autoLaunch);
            return 1;
        } else {
            LOGI("[+] Non-zygote process detected, doing normal injection");

            if (data->bypassNamespaceRestrictions) {
                LOGI("[+] Bypassing namespace restrictions");
                void *xdl_handle = xdl_open(reinterpret_cast<const char *>(data->libraryPath), XDL_ALWAYS_FORCE_LOAD);
                if (xdl_handle == nullptr) {
                    LOGE("Error dlopening library %s: %s", data->libraryPath, dlerror());
                    return 1;
                }

                LOGI("[+] Library loaded successfully");
                return 1;
            }

            return JNIProxy::Inject(data);
        }

        return 1;
    }
}