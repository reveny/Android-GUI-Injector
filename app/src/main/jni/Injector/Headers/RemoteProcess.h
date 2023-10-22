#pragma once

#include <cstdarg>
#include <utility>

#include "xptrace.h"

class RemoteProcess {
private:
#if defined(__arm__) || defined(__aarch64__)
    static constexpr auto CPSR_T_MASK = (1u << 5);
#endif
    #if defined(__arm__)
        #define CPSR_T_MASK (1u << 5)
        #define PARAMS_IN_REGS 4
    #elif defined(__aarch64__)
        #define PARAMS_IN_REGS 8
        #define pt_regs user_pt_regs
        #define uregs regs
        #define ARM_pc pc
        #define ARM_sp sp
        #define ARM_cpsr pstate
        #define ARM_lr regs[30]
        #define ARM_r0 regs[0]
        #define PTRACE_GETREGS PTRACE_GETREGSET
        #define PTRACE_SETREGS PTRACE_SETREGSET
    #endif

    int pid;

    bool _remote_read(uintptr_t addr, void *buf, size_t len) const;
    bool _remote_write(uintptr_t addr, const void *buf, size_t len) const;
    bool _remote_getregs(pt_regs *regs) const;
    bool _remote_setregs(pt_regs *regs) const;
    uintptr_t remote_call_abi(uintptr_t func_addr, int nargs, va_list va) const;
    uintptr_t call_absolute(uintptr_t addr, int nargs, ...) const;
    uintptr_t call_vararg(uintptr_t addr, int nargs, ...) const;
public:
    explicit RemoteProcess(int pid);

    pid_t get_pid() const;
    uintptr_t remote_string(const char* string);

    bool write(uintptr_t addr, const void *buf, size_t len) const;
    bool read(uintptr_t addr, void *buf, size_t len) const;

    // Call Template
    template<class FuncPtr, class ...Args>
    uintptr_t call(FuncPtr sym, Args && ...args) const {
        auto addr = reinterpret_cast<uintptr_t>(sym);
        return call_vararg(addr, sizeof...(args), std::forward<Args>(args)...);
    }

    // Absolute Call Template
    template<class FuncPtr, class ...Args>
    uintptr_t a_call(FuncPtr sym, Args && ...args) const {
        auto addr = reinterpret_cast<uintptr_t>(sym);
        return call_absolute(addr, sizeof...(args), std::forward<Args>(args)...);
    }
};
