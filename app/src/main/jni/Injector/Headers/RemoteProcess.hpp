//
// Created by reveny on 5/17/24.
//
#pragma once

#include <cstdarg>
#include <utility>

#include <xptrace.hpp>

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

    bool RemoteRead(uintptr_t addr, void *buf, size_t len);
    bool RemoteWrite(uintptr_t addr, const void *buf, size_t len);

    bool RemoteGetregs(pt_regs *regs);
    bool RemoteSetregs(pt_regs *regs);

    uintptr_t RemoteCall(uintptr_t func_addr, int nargs, va_list va);
    uintptr_t CallAbsolute(uintptr_t addr, int nargs, ...);
    uintptr_t CallVararg(uintptr_t addr, int nargs, ...);

public:
    RemoteProcess(pid_t pid);

    pid_t GetPID();
    uintptr_t RemoteString(std::string string);

    bool Write(uintptr_t addr, const void *buf, size_t len);
    bool Read(uintptr_t addr, void *buf, size_t len);

    // Call Template
    template<class FuncPtr, class ...Args>
    uintptr_t call(FuncPtr sym, Args && ...args) {
        auto addr = reinterpret_cast<uintptr_t>(sym);
        return CallVararg(addr, sizeof...(args), std::forward<Args>(args)...);
    }

    // Absolute Call Template
    template<class FuncPtr, class ...Args>
    uintptr_t a_call(FuncPtr sym, Args && ...args) {
        auto addr = reinterpret_cast<uintptr_t>(sym);
        return CallAbsolute(addr, sizeof...(args), std::forward<Args>(args)...);
    }
};
