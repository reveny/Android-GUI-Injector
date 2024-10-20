//
// Created by reveny on 5/17/24.
//
#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>

#include <Logger.hpp>
#include <RemoteProcess.hpp>
#include <InjectorData.hpp>
#include <ELFUtil.hpp>

namespace RevMemory {
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

    int Inject(pid_t pid, std::string library, bool remap);
    int EmulatorInject(pid_t pid, std::string library, bool remap);

    int InjectLinkerBypass(std::shared_ptr<RemoteProcess> process, std::string libraryPath, std::shared_ptr<InjectorData> data);
    int InjectMemfdDlopen(std::shared_ptr<RemoteProcess> process, std::string libraryPath, std::shared_ptr<InjectorData> data);
    int InjectProxy(pid_t pid, std::string libraryPath, std::shared_ptr<InjectorData> data);

    uintptr_t CopyInjectorData(std::shared_ptr<RemoteProcess> process, std::shared_ptr<InjectorData> data);
    bool ProcessVirtualMemory(pid_t pid, void *address, void *buffer, size_t size, bool iswrite);

    void LaunchApp(std::string activity);
    pid_t WaitForProcess(std::string packageName);
    pid_t FindProcessID(std::string packageName);

    void SetSELinux(int enabled);
    std::string GetNativeLibraryDirectory();

    uintptr_t GetModuleBase(pid_t pid, std::string loduleName);
    void RemoteRemap(std::shared_ptr<RemoteProcess> process, std::string libraryName);

    std::string GetRemoteModuleName(pid_t pid, uintptr_t addr);
    uintptr_t GetRemoteFunctionAddr(pid_t pid, std::string moduleName, uintptr_t localFuncAddr);

    template<typename T>
    T Read(pid_t pid, uintptr_t address, size_t size = 0) {
        size_t tmp_size = size;
        if (tmp_size == 0) {
            tmp_size = sizeof(T);
        }

        T data;
        ProcessVirtualMemory(pid, reinterpret_cast<void *>(address), reinterpret_cast<void *>(&data), sizeof(T), false);
        return data;
    }

    template<typename T>
    void Write(pid_t pid, uintptr_t address, T data) {
        ProcessVirtualMemory(pid, (void *) address, reinterpret_cast<void *>(&data), sizeof(T), true);
    }
};
