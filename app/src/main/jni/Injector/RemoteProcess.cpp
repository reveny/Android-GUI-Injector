#include "Headers/RemoteProcess.h"
#include "Headers/RevMemory.h"
#include "Headers/xptrace.h"
#include <elf.h>
#include <sys/mount.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <cstring>
#include <unistd.h>

using namespace std;

RemoteProcess::RemoteProcess(int pid) : pid(pid) {}

pid_t RemoteProcess::get_pid() const {
    return pid;
}

uintptr_t RemoteProcess::remote_string(const char* string) {
    // Allocate Memory and write target library path to it
    uintptr_t map = call(malloc, 256);
    if (map == -1) {
        LOGE("[-] Failed to allocate memory for string");
        return -1;
    }

    bool w = write(map, string, strlen(string));
    if (!w) {
        LOGE("[-] Failed to write path %s to address %p", string, map);
        return -1;
    }
    LOGI("[+] Successfully wrote string to address %p", map);
    return map;
}

bool RemoteProcess::_remote_read(uintptr_t addr, void *buf, size_t len) const {
    for (size_t i = 0; i < len; i += sizeof(long)) {
        long data = xptrace(PTRACE_PEEKTEXT, pid, reinterpret_cast<void*>(addr + i));
        if (data < 0) {
            return false;
        }
        memcpy(static_cast<uint8_t *>(buf) + i, &data, std::min(len - i, sizeof(data)));
    }
    return true;
}

bool RemoteProcess::_remote_write(uintptr_t addr, const void *buf, size_t len) const {
    for (size_t i = 0; i < len; i += sizeof(long)) {
        long data = 0;
        memcpy(&data, static_cast<const uint8_t *>(buf) + i, std::min(len - i, sizeof(data)));
        if (xptrace(PTRACE_POKETEXT, pid, reinterpret_cast<void*>(addr + i), data) < 0) {
            return false;
        }
    }
    return true;
}

bool RemoteProcess::_remote_getregs(pt_regs *regs) const {
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

bool RemoteProcess::_remote_setregs(pt_regs *regs) const {
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

uintptr_t RemoteProcess::remote_call_abi(uintptr_t func_addr, int nargs, va_list va) const {
    pt_regs regs{}, originalRegs{};

    if (!_remote_getregs(&regs)) {
        LOGE("[-] Remote Call failed because it failed to get the current regs");
        return -1;
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
            regs.ARM_sp -= sizeof(uintptr_t) * (nargs - PARAMS_IN_REGS);
            uintptr_t stack = regs.ARM_sp;
            for (int i = PARAMS_IN_REGS; i < nargs; ++i) {
                uintptr_t arg = va_arg(va, uintptr_t);
                if (!_remote_write(stack, &arg, sizeof(uintptr_t))) {
                    LOGE("[-] Remote Call Failed because remote write arguments failed");
                    return -1;
                }
                stack += sizeof(uintptr_t);
            }
        }

        regs.ARM_pc = func_addr;
        if (regs.ARM_pc & 1){
            regs.ARM_pc &= (~1u);
            regs.ARM_cpsr |= CPSR_T_MASK;
        } else {
            regs.ARM_cpsr &= ~CPSR_T_MASK;
        }

        regs.ARM_lr = 0;
    #elif defined(__i386__)
        // Push all params onto stack
        regs.esp -= sizeof(uintptr_t) * nargs;
        uintptr_t stack = regs.esp;
        for (int i = 0; i < nargs; ++i) {
            uintptr_t arg = va_arg(va, uintptr_t);
            _remote_write(stack, &arg, sizeof(uintptr_t));
            stack += sizeof(uintptr_t);
        }

        // Push return address onto stack
        uintptr_t ret_addr = 0;
        regs.esp -= sizeof(uintptr_t);
        _remote_write(regs.esp, &ret_addr, sizeof(uintptr_t));

        // Set function address to call
        regs.eip = func_addr;
    #elif defined(__x86_64__)
        // Align, rsp - 8 must be a multiple of 16 at function entry point
        uintptr_t space = sizeof(uintptr_t);
        if (nargs > 6) {
            space += sizeof(uintptr_t) * (nargs - 6);
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
            regs.rsp -= sizeof(uintptr_t) * (nargs - 6);
            uintptr_t stack = regs.rsp;
            for (int i = 6; i < nargs; ++i) {
                uintptr_t arg = va_arg(va, uintptr_t);
                _remote_write(stack, &arg, sizeof(uintptr_t));
                stack += sizeof(uintptr_t);
            }
        }

        // Push return address onto stack
        uintptr_t ret_addr = 0;
        regs.rsp -= sizeof(uintptr_t);
        _remote_write(regs.rsp, &ret_addr, sizeof(uintptr_t));

        // Set function address to call
        regs.rip = func_addr;

        // may be needed
        regs.rax = 0;
        regs.orig_rax = 0;
    #else
        #error Unsupported Device
    #endif

    if (!_remote_setregs(&regs) || xptrace(PTRACE_CONT, pid) == -1) {
        return -1;
    }

    int stat = 0;
    waitpid(pid, &stat, WUNTRACED);

    while ((stat & 0xFF) != 0x7f) {
        if (xptrace(PTRACE_CONT, pid) == -1) {
            return -1;
        }
        waitpid(pid, &stat, WUNTRACED);
    }

    if (!_remote_getregs(&regs)) {
        LOGE("[-] Remote Call Failed because it could not get the regs");
        return -1;
    }

    // Restore the registers to the original we backed up earlier
    if (!_remote_setregs(&originalRegs)) {
        LOGE("[-] Remote Call Failed because it could not set the original regs");
        return -1;
    }

    // Get Return Value
    #if defined(__arm__) || defined(__aarch64__)
        return regs.ARM_r0;
    #elif defined(__i386__)
        return regs.eax;
    #elif defined(__x86_64__)
        return regs.rax;
    #endif
}

uintptr_t RemoteProcess::call_absolute(uintptr_t addr, int nargs, ...) const {
    va_list va;
    va_start(va, nargs);
    auto result = remote_call_abi((uintptr_t)addr, nargs, va);
    va_end(va);
    return result;
}

uintptr_t RemoteProcess::call_vararg(uintptr_t addr, int nargs, ...) const {
    // Get Module Name from Address
    const char *mod_name = RevMemory::get_remote_module_name(getpid(), addr);

    if (string(mod_name).empty()) {
        LOGE("[-] Failed To Acquire module name for address %lu", addr);
        return 0x0;
    }
    LOGI("[+] Found Library for address %p, %s", addr, mod_name);

    // Get Call Address
    void *remote = RevMemory::get_remote_func_addr(pid, mod_name, reinterpret_cast<void *>(addr));
    LOGI("[+] Found remote address %p in pid %d", remote, pid);
  
    va_list va;
    va_start(va, nargs);
    auto result = remote_call_abi((uintptr_t)remote, nargs, va);
    va_end(va);
    return result;
}

bool RemoteProcess::write(uintptr_t addr, const void *buf, size_t len) const {
    return _remote_write(addr, buf, len);
}

bool RemoteProcess::read(uintptr_t addr, void *buf, size_t len) const {
    return _remote_read(addr, buf, len);
}