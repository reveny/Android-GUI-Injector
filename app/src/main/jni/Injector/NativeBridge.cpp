//
// Created by reveny on 5/17/24.
//

#include <sys/system_properties.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <jni.h>

#include <NativeBridge.hpp>
#include <RevMemory.hpp>
#include <sys/mman.h>

std::string NativeBridge::GetNativeBridge() {
    auto value = std::array<char, PROP_VALUE_MAX>();
    __system_property_get("ro.dalvik.vm.native.bridge", value.data());
    return {value.data()};
}

int NativeBridge::LoadNativeBridge(std::shared_ptr<RemoteProcess> process, void *data, size_t length) {
    sleep(1);

    errno = 0; // Reset Errno
    uintptr_t native_bridge = process->call(dlopen, process->RemoteString("libhoudini.so"), RTLD_NOW);
    if (native_bridge == 0x0) {
        // Check which library is responsible for native bridge
        std::string native = GetNativeBridge();
        LOGI("[+] libhoudini.so does not exist, using %s instead (Error: %s)", native.c_str(), strerror(errno));

        // Set new native bridge
        native_bridge = process->call(dlopen, process->RemoteString(native.c_str()), RTLD_NOW);
    }

    if (native_bridge == 0x0) {
        LOGE("[-] No native bridge library found, is arm translation available on this emulator?");
        return -1;
    }
    LOGI("[+] Got native bridge handle: %p", native_bridge);

    // Get native bridge structure
    uintptr_t nb_callbacks = process->call(dlsym, native_bridge, process->RemoteString("NativeBridgeItf"));
    if (nb_callbacks) {
        // Read libhoudini version
        int version = RevMemory::Read<int>(process->GetPID(), nb_callbacks, sizeof(uint32_t));
        LOGI("[+] Found libhoudini version: %d", version);

        // Since we can't just access the struct, we need to read the data from memory
        NativeBridgeCallbacks nb_local_read = RevMemory::Read<NativeBridgeCallbacks>(process->GetPID(), nb_callbacks, sizeof(uintptr_t) * 18);

        LOGI("[+] NativeBridge: loadLibrary: %p", nb_local_read.loadLibrary);
        LOGI("[+] NativeBridge: loadLibraryExt: %p", nb_local_read.loadLibraryExt);
        LOGI("[+] NativeBridge: getTrampoline: %p", nb_local_read.getTrampoline);

        int fd = (int) process->call(syscall, process_memfd_create, process->RemoteString("anon"), MFD_CLOEXEC);
        (void)process->call(ftruncate, fd, (off_t)length);

        uintptr_t mem = process->call(mmap, nullptr, length, PROT_WRITE, MAP_SHARED, fd, 0);
        if (mem == 0x0) {
            LOGE("[-] NativeBridge: Failed to allocate memory");
            return -1;
        }

        (void)process->call(memcpy, mem, data, length);
        (void)process->call(munmap, mem, length);
        (void)process->call(munmap, data, length);

        std::string path = "/proc/" + std::to_string(process->GetPID()) + "/fd/" + std::to_string(fd);
        LOGI("[+] NativeBridge: Path: %s", path.c_str());

        // Changes in android 8 and above
        errno = 0;
        uintptr_t library_handle = 0x0;
        int api_level = android_get_device_api_level();
        if (api_level >= 26) {
            library_handle = process->a_call(nb_local_read.loadLibraryExt, process->RemoteString(path.c_str()), RTLD_GLOBAL | RTLD_NOW, reinterpret_cast<void *>(3));
        } else {
            library_handle = process->a_call(nb_local_read.loadLibrary, process->RemoteString(path.c_str()), RTLD_GLOBAL | RTLD_NOW);
        }

        if (library_handle != 0x0 && library_handle != 0x1) {
            LOGI("[+] NativeBridge: Load library success, handle: %p", library_handle);
        } else {
            LOGE("[-] NativeBridge: Load library failed, handle: %p error: %s", library_handle, strerror(errno));
            return -1;
        }

        // No JNI_OnLoad support here for now, use proxy if you need the jni calling.
        // I generally recommend to use proxy instead of native bridge for emulator injection.
    }

    return 1;
}