//
// Created by reveny on 21/10/2023.
// Mostly taken from ZygiskIl2cppDumper by Perfare
//

#ifndef DLL_INJECTOR_V2_NATIVEBRIDGE_H
#define DLL_INJECTOR_V2_NATIVEBRIDGE_H

#include <sys/system_properties.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <jni.h>

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

    static std::string get_native_bridge() {
        auto value = std::array<char, PROP_VALUE_MAX>();
        __system_property_get("ro.dalvik.vm.native.bridge", value.data());
        return {value.data()};
    }

    static int LoadNativeBridge(RemoteProcess process, void* data, size_t length) {
        sleep(1);

        errno = 0; // Reset Errno
        uintptr_t native_bridge = process.call(dlopen, process.remote_string("libhoudini.so"), RTLD_NOW);
        if (native_bridge == 0x0) {
            // Check which library is responsible for native bridge
            std::string native = get_native_bridge();
            LOGI("[+] libhoudini.so does not exist, using %s instead (Error: %s)", native.c_str(), strerror(errno));

            // Set new native bridge
            native_bridge = process.call(dlopen, process.remote_string(native.c_str()), RTLD_NOW);
        }

        if (native_bridge == 0x0) {
            LOGE("[-] No native bridge library found, is arm translation available on this emulator?");
            return -1;
        }
        LOGI("[+] Got native bridge handle: %p", native_bridge);

        // Get native bridge structure
        uintptr_t nb_callbacks = process.call(dlsym, native_bridge, process.remote_string("NativeBridgeItf"));
        if (nb_callbacks) {
            // Read libhoudini version
            int version = RevMemory::Read<int>(process.get_pid(), nb_callbacks, sizeof(uint32_t));
            LOGI("[+] Found libhoudini version: %d", version);

            // Since we can't just access the struct, we need to read the data from memory
            NativeBridgeCallbacks nb_local_read = RevMemory::Read<NativeBridgeCallbacks>(process.get_pid(), nb_callbacks, sizeof(uintptr_t) * 18);

            LOGI("[+] NativeBridge: loadLibrary: %p", nb_local_read.loadLibrary);
            LOGI("[+] NativeBridge: loadLibraryExt: %p", nb_local_read.loadLibraryExt);
            LOGI("[+] NativeBridge: getTrampoline: %p", nb_local_read.getTrampoline);

            int fd = (int) process.call(syscall, process_memfd_create, process.remote_string("anon"), MFD_CLOEXEC);
            (void)process.call(ftruncate, fd, (off_t)length);

            uintptr_t mem = process.call(mmap, nullptr, length, PROT_WRITE, MAP_SHARED, fd, 0);
            if (mem == 0x0) {
                LOGE("[-] NativeBridge: Failed to allocate memory");
                return -1;
            }

            (void)process.call(memcpy, mem, data, length);
            (void)process.call(munmap, mem, length);
            (void)process.call(munmap, data, length);

            std::string path = "/proc/" + std::to_string(process.get_pid()) + "/fd/" + std::to_string(fd);
            LOGI("[+] NativeBridge: Path: %s", path.c_str());

            // Changes in android 8 and above
            errno = 0;
            uintptr_t library_handle = 0x0;
            int api_level = android_get_device_api_level();
            if (api_level >= 26) {
                library_handle = process.a_call(nb_local_read.loadLibraryExt, process.remote_string(path.c_str()), RTLD_GLOBAL | RTLD_NOW, reinterpret_cast<void *>(3));
            } else {
                library_handle = process.a_call(nb_local_read.loadLibrary, process.remote_string(path.c_str()), RTLD_GLOBAL | RTLD_NOW);
            }

            if (library_handle != 0x0 && library_handle != 0x1) {
                LOGI("[+] NativeBridge: Load library success, handle: %p", library_handle);
            } else {
                LOGE("[-] NativeBridge: Load library failed, handle: %p error: %s", library_handle, strerror(errno));
                return -1;
            }

            // TODO: JNI_OnLoad support
        }

        return 1;
    }
}

#endif
