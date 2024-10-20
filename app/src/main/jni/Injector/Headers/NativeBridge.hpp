//
// Created by reveny on 5/17/24.
//
#pragma once

#include <string>
#include "RemoteProcess.hpp"

namespace NativeBridge {
    struct NativeBridgeCallbacks {
        uint32_t version;
        void *initialize;

        void *(*loadLibrary)(const char *libpath, int flag);
        void *(*getTrampoline)(void *handle, const char *name, const char *shorty, uint32_t len);

        void *isSupported;
        void *getAppEnv;
        void *isCompatibleWith;
        void *getSignalHandler;
        void *unloadLibrary;
        void *getError;
        void *isPathSupported;
        void *initAnonymousNamespace;
        void *createNamespace;
        void *linkNamespaces;

        void *(*loadLibraryExt)(const char *libpath, int flag, void *ns);
    };

    std::string GetNativeBridge();
    int LoadNativeBridge(std::shared_ptr<RemoteProcess> process, void* data, size_t length);
}
