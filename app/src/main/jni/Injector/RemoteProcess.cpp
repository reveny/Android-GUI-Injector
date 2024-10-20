//
// Created by reveny on 5/17/24.
//

#include <elf.h>
#include <sys/mount.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <cstring>
#include <unistd.h>

#include <RemoteProcess.hpp>
#include <RevMemory.hpp>
#include <xptrace.hpp>

RemoteProcess::RemoteProcess(pid_t pid) : pid(pid) {}

pid_t RemoteProcess::GetPID() {
    return pid;
}

uintptr_t RemoteProcess::RemoteString(std::string string) {
    // Allocate Memory and write target library path to it
    uintptr_t map = call(malloc, string.size() + 1);
    if (static_cast<int>(map) == -1) {
        LOGE("[-] Failed to allocate memory for string");
        return static_cast<uintptr_t>(-1);
    }

    bool w = Write(map, string.data(), string.size());
    if (!w) {
        LOGE("[-] Failed to write path %s to address %p", string.c_str(), map);
        return static_cast<uintptr_t>(-1);
    }
    LOGI("[+] Successfully wrote string to address %p", map);
    return map;
}

bool RemoteProcess::Write(uintptr_t addr, const void *buf, size_t len) {
    return RemoteWrite(addr, buf, len);
}

bool RemoteProcess::Read(uintptr_t addr, void *buf, size_t len) {
    return RemoteRead(addr, buf, len);
}

bool RemoteProcess::RemoteRead(uintptr_t addr, void *buf, size_t len) {
    for (size_t i = 0; i < len; i += sizeof(long)) {
        long data = xptrace(PTRACE_PEEKTEXT, pid, reinterpret_cast<void*>(addr + i));
        if (data < 0) {
            return false;
        }
        memcpy(static_cast<uint8_t *>(buf) + i, &data, std::min(len - i, sizeof(data)));
    }
    return true;
}

bool RemoteProcess::RemoteWrite(uintptr_t addr, const void *buf, size_t len) {
    for (size_t i = 0; i < len; i += sizeof(long)) {
        long data = 0;
        memcpy(&data, static_cast<const uint8_t *>(buf) + i, std::min(len - i, sizeof(data)));
        if (xptrace(PTRACE_POKETEXT, pid, reinterpret_cast<void*>(addr + i), data) < 0) {
            return false;
        }
    }
    return true;
}

bool RemoteProcess::RemoteGetregs(pt_regs *regs) {
    #if defined(__LP64__)
        iovec iov{};
        iov.iov_base = regs;
        iov.iov_len = sizeof(*regs);
        if (xptrace(PTRACE_GETREGSET, pid, reinterpret_cast<void*>(NT_PRSTATUS), &iov) == -1) {
            LOGE("[-] Could not get regs: %s", strerror(errno));
            return false;
        }
    #else
        if (xptrace(PTRACE_GETREGS, pid, nullptr, regs) == -1) {
            LOGE("[-] Could not get regs: %s", strerror(errno));
            return false;
        }
    #endif

    return true;
}

bool RemoteProcess::RemoteSetregs(pt_regs *regs) {
    #if defined(__LP64__)
        iovec iov{};
        iov.iov_base = regs;
        iov.iov_len = sizeof(*regs);
        if (xptrace(PTRACE_SETREGSET, pid, reinterpret_cast<void*>(NT_PRSTATUS), &iov) == -1) {
            LOGE("[-] Could not get regs: %s", strerror(errno));
            return false;
        }
    #else
        if (xptrace(PTRACE_SETREGS, pid, nullptr, regs) == -1) {
            LOGE("[-] Could not get regs: %s", strerror(errno));
            return false;
        }
    #endif

    return true;
}

uintptr_t RemoteProcess::RemoteCall(uintptr_t functionAddr, int nargs, va_list va) {
    pt_regs regs{}, originalRegs{};

    if (!RemoteGetregs(&regs)) {
        LOGE("[-] Remote Call failed because it failed to get the current regs");
        return static_cast<uintptr_t>(-1);
    }

    // Backup Original Register
    memcpy(&originalRegs, &regs, sizeof(regs));

    // ABI dependent: Setup stack and registers to perform the call
    #if defined(__arm__) || defined(__aarch64__)
        int i = 0;
        for (i = 0; (i < nargs) && (i < PARAMS_IN_REGS); ++i) {
            regs.uregs[i] = va_arg(va, uintptr_t);
        }

        if (nargs > PARAMS_IN_REGS) {
            regs.ARM_sp -= sizeof(uintptr_t) * (static_cast<unsigned long>(nargs - PARAMS_IN_REGS));
            uintptr_t stack = regs.ARM_sp;
            for (i = PARAMS_IN_REGS; i < nargs; ++i) {
                uintptr_t arg = va_arg(va, uintptr_t);
                if (!RemoteWrite(stack, &arg, sizeof(uintptr_t))) {
                    LOGE("[-] Remote Call Failed because remote write arguments failed");
                    return static_cast<uintptr_t>(-1);
                }
                stack += sizeof(uintptr_t);
            }
        }

        regs.ARM_pc = functionAddr;
        if (regs.ARM_pc & 1){
            regs.ARM_pc &= (~1u);
            regs.ARM_cpsr |= CPSR_T_MASK;
        } else {
            regs.ARM_cpsr &= ~CPSR_T_MASK;
        }

        regs.ARM_lr = 0;
    #elif defined(__i386__)
        // Push all params onto stack
        regs.esp -= sizeof(uintptr_t) * static_cast<unsigned int>(nargs);
        long stack = regs.esp;
        for (int i = 0; i < nargs; ++i) {
            uintptr_t arg = va_arg(va, uintptr_t);
            RemoteWrite(static_cast<uintptr_t>(stack), &arg, sizeof(uintptr_t));
            stack += sizeof(uintptr_t);
        }

        // Push return address onto stack
        uintptr_t ret_addr = 0;
        regs.esp -= sizeof(uintptr_t);
        RemoteWrite(static_cast<uintptr_t>(regs.esp), &ret_addr, sizeof(uintptr_t));

        // Set function address to call
        regs.eip = static_cast<long>(functionAddr);
    #elif defined(__x86_64__)
        // Align, rsp - 8 must be a multiple of 16 at function entry point
        uintptr_t space = sizeof(uintptr_t);
        if (nargs > 6) {
            space += sizeof(uintptr_t) * (static_cast<unsigned long>(nargs - 6));
        }
        while (((regs.rsp - space - 8) & 0xF) != 0) {
            regs.rsp--;
        }

        // Fill [RDI, RSI, RDX, RCX, R8, R9] with the first 6 parameters
        for (int i = 0; (i < nargs) && (i < 6); ++i) {
            uintptr_t arg = va_arg(va, uintptr_t);
            switch (i) {
                case 0: regs.rdi = arg; break;
                case 1: regs.rsi = arg; break;
                case 2: regs.rdx = arg; break;
                case 3: regs.rcx = arg; break;
                case 4: regs.r8 = arg; break;
                case 5: regs.r9 = arg; break;
            }
        }

        // Push remaining parameters onto stack
        if (nargs > 6) {
            regs.rsp -= sizeof(uintptr_t) * (static_cast<unsigned long>(nargs - 6));
            uintptr_t stack = regs.rsp;
            for (int i = 6; i < nargs; ++i) {
                uintptr_t arg = va_arg(va, uintptr_t);
                RemoteWrite(stack, &arg, sizeof(uintptr_t));
                stack += sizeof(uintptr_t);
            }
        }

        // Push return address onto stack
        uintptr_t ret_addr = 0;
        regs.rsp -= sizeof(uintptr_t);
        RemoteWrite(regs.rsp, &ret_addr, sizeof(uintptr_t));

        // Set function address to call
        regs.rip = functionAddr;

        // may be needed
        regs.rax = 0;
        regs.orig_rax = 0;
    #else
        #error Unsupported Device
    #endif

    if (!RemoteSetregs(&regs) || xptrace(PTRACE_CONT, pid) == -1) {
        return static_cast<uintptr_t>(-1);
    }

    int stat = 0;
    waitpid(pid, &stat, WUNTRACED);

    while ((stat & 0xFF) != 0x7f) {
        if (xptrace(PTRACE_CONT, pid) == -1) {
            return static_cast<uintptr_t>(-1);
        }
        waitpid(pid, &stat, WUNTRACED);
    }

    if (!RemoteGetregs(&regs)) {
        LOGE("[-] Remote Call Failed because it could not get the regs");
        return static_cast<uintptr_t>(-1);
    }

    // Restore the registers to the original we backed up earlier
    if (!RemoteSetregs(&originalRegs)) {
        LOGE("[-] Remote Call Failed because it could not set the original regs");
        return static_cast<uintptr_t>(-1);
    }

    // Get Return Value
    #if defined(__arm__) || defined(__aarch64__)
        return regs.ARM_r0;
    #elif defined(__i386__)
        return static_cast<uintptr_t>(regs.eax);
    #elif defined(__x86_64__)
        return regs.rax;
    #endif
}

uintptr_t RemoteProcess::CallAbsolute(uintptr_t addr, int nargs, ...) {
    va_list va;
    va_start(va, nargs);
    auto result = RemoteCall(addr, nargs, va);
    va_end(va);
    return result;
}

uintptr_t RemoteProcess::CallVararg(uintptr_t addr, int nargs, ...) {
    // Get Module Name from Address
    std::string moduleName = RevMemory::GetRemoteModuleName(getpid(), addr);

    if (moduleName.empty()) {
        LOGE("[-] Failed To Acquire module name for address %lu", addr);
        return 0x0;
    }
    LOGI("[+] Found Library for address %p, %s", addr, moduleName.c_str());

    // Get Call Address
    uintptr_t remote = RevMemory::GetRemoteFunctionAddr(pid, moduleName, addr);
    LOGI("[+] Found remote address %p in pid %d", remote, pid);
  
    va_list va;
    va_start(va, nargs);
    auto result = RemoteCall((uintptr_t)remote, nargs, va);
    va_end(va);
    return result;
}