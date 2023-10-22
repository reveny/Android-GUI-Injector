#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>

#include "../../Include/Logger.h"
#include "../Headers/RemoteProcess.h"
#include "../Headers/ELFUtil.h"

class RevMemory {
public:
    struct ProcMapInfo {
        uintptr_t start;
        uintptr_t end;
        uintptr_t offset;
        uint8_t perms;
        ino_t inode;
        char* dev;
        char* path;
    };

    // https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md
    #if defined(__arm__)
        #define process_vm_readv_syscall 376
        #define process_vm_writev_syscall 377
        #define process_memfd_create 385
    #elif defined(__aarch64__)
        #define process_vm_readv_syscall 270
        #define process_vm_writev_syscall 271
        #define process_memfd_create 279
    #elif defined(__i386__)
        #define process_vm_readv_syscall 347
        #define process_vm_writev_syscall 348
        #define process_memfd_create 356
    #else
        #define process_vm_readv_syscall 310
        #define process_vm_writev_syscall 311
        #define process_memfd_create 319
    #endif

    static int Inject(pid_t pid, const char* library, bool remap);
    static int EmulatorInject(pid_t pid, const char* library, bool remap);

    static bool process_virtual_memory(pid_t pid, void *address, void *buffer, size_t size, bool iswrite);
    static void launch_app(const char *activity);
    static int find_process_id(const char *packageName);
    static void set_selinux(int enabled);
    static void* get_module_base_addr(pid_t pid, const char* loduleName);
    static std::vector<RevMemory::ProcMapInfo> list_modules(pid_t pid, const char* library);
    static void remote_remap(RemoteProcess process, const char* library_name);
    static const char *get_remote_module_name(pid_t pid, uintptr_t addr);
    static void* get_remote_func_addr(pid_t pid, const char* moduleName, void* localFuncAddr);

    template<typename T>
    static T Read(pid_t pid, uintptr_t address, size_t size = 0) {
        size_t tmp_size = size;
        if (tmp_size == 0) {
            tmp_size = sizeof(T);
        }

        T data;
        process_virtual_memory(pid, reinterpret_cast<void *>(address), reinterpret_cast<void *>(&data), sizeof(T), false);
        return data;
    }

    template<typename T>
    static void Write(pid_t pid, uintptr_t address, T data) {
        process_virtual_memory(pid, (void *) address, reinterpret_cast<void *>(&data), sizeof(T), true);
    }
};
