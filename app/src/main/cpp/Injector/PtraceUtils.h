#include <asm/ptrace.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <elf.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

#include "Utils.h"

#if defined(__aarch64__) //x64
#define pt_regs user_pt_regs
#define uregs regs
#define ARM_pc pc
#define ARM_sp sp
#define ARM_cpsr pstate
#define ARM_lr regs[30]
#define ARM_r0 regs[0]
#define PTRACE_GETREGS PTRACE_GETREGSET
#define PTRACE_SETREGS PTRACE_SETREGSET
#elif defined(__x86_64__) //x86_64
#define pt_regs user_regs_struct
#define eax rax
#define esp rsp
#define eip rip
#elif defined(__i386__) //x86
#define pt_regs user_regs_struct
#endif

// rest predefined
#define CPSR_T_MASK (1u << 5)

//Attach ptrace to process using the processID
int ptraceAttach(pid_t pid) {
    int status = 0;
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        LOGE("ptraceAttach error, pid: %d, error: %s", pid, strerror(errno));
        return -1;
    }

    LOGI("ptraceAttach success");
    waitpid(pid, &status, WUNTRACED);

    return 0;
}

//Used to the attached process doesn't stop
int ptraceContinue(pid_t pid) {
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) < 0) {
        LOGE("ptraceContinue error, pid: %d, error: %s", pid, strerror(errno));
        return -1;
    }

    LOGI("ptraceContinue success");
    return 0;
}

//Detach from the target process
int ptraceDetach(pid_t pid) {
    if (ptrace(PTRACE_DETACH, pid, NULL, 0) < 0) {
        LOGE("ptraceDetach error, pid: %d, err: %s", pid, strerror(errno));
        return -1;
    }

    LOGI("ptraceDetach success");
    return 0;
}

//get the register value of the process
int ptrace_getregs(pid_t pid, struct pt_regs *regs) {
    #if defined(__aarch64__)
        int regset = NT_PRSTATUS;
        struct iovec ioVec;

        ioVec.iov_base = regs;
        ioVec.iov_len = sizeof(*regs);
        if (ptrace(PTRACE_GETREGSET, pid, (void *)regset, &ioVec) < 0){
            LOGE("ptrace_getregs: Can not get register values, io %llx, %d\n", ioVec.iov_base,ioVec.iov_len);
            return -1;
        }

        return 0;
    #else
        if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0) {
            LOGE("Get Ptrace regs error, error: %s\n", strerror(errno));
            return -1;
        }
    #endif
    return 0;
}

//set the register value of the process
int ptrace_setregs(pid_t pid, struct pt_regs *regs){
    #if defined(__aarch64__)
        int regset = NT_PRSTATUS;
        struct iovec ioVec;

        ioVec.iov_base = regs;
        ioVec.iov_len = sizeof(*regs);
        if (ptrace(PTRACE_SETREGSET, pid, (void *)regset, &ioVec) < 0) {
            LOGE("ptrace_setregs: Can not get register values");
            return -1;
        }

        return 0;
    #else
        if (ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0) {
            LOGE("Set Regs error, pid:%d, err:%s\n", pid, strerror(errno));
            return -1;
        }
    #endif
    return 0;
}

//Return value of any function in the process
//Return value is stored in the corresponding register
long ptrace_getret(struct pt_regs *regs) {
    #if defined(__i386__) || defined(__x86_64__) //
        return regs->eax;
    #elif defined(__arm__) || defined(__aarch64__) //
        return regs->ARM_r0;
    #else
        LOGE("Device not supported");
    #endif
}

long ptrace_getpc(struct pt_regs *regs) {
    #if defined(__i386__) || defined(__x86_64__)
        return regs->eip;
    #elif defined(__arm__) || defined(__aarch64__)
        return regs->ARM_pc;
    #else
        LOGE("Device not supported");
    #endif
}

//get the dlopen address of the target process
void *getDlOpenAddr(pid_t pid) {
    void *dlOpenAddress;

    //Emulators such as noxplayer have different sdk versions
    //This works for android 5 - 7
    //If the emulator has a higher android version, adjust this
    if (arch == deviceArchitecture::x86 || arch == deviceArchitecture::x86_64) {
        dlOpenAddress = getRemoteFuncAddr(pid, linkerPath, (void *) dlopen);
    } else {
        if (sdkVersion <= 23) { // Android 7
            dlOpenAddress = getRemoteFuncAddr(pid, linkerPath, (void *) dlopen);
        } else {
            dlOpenAddress = getRemoteFuncAddr(pid, libDLPath, (void *) dlopen);
        }
    }

    LOGE("dlopen getRemoteFuncAddr: 0x%lx", (uintptr_t)dlOpenAddress);
    return dlOpenAddress;
}

//get the dlsym address of the target process
//can be used in the future to call a specific function in the injected process
void *getDlsymAddr(pid_t pid) {
    void *dlSymAddress;

    if (arch == deviceArchitecture::x86 || arch == deviceArchitecture::x86_64) {
        dlSymAddress = getRemoteFuncAddr(pid, linkerPath, (void *) dlopen);
    } else {
        if (sdkVersion <= 23) {
            dlSymAddress = getRemoteFuncAddr(pid, linkerPath, (void *) dlsym);
        } else {
            dlSymAddress = getRemoteFuncAddr(pid, libDLPath, (void *) dlsym);
        }
    }

    LOGE("dlsym getRemoteFuncAddr: 0x%lx", (uintptr_t) dlSymAddress);
    return dlSymAddress;
}

//get the dlerror address of the target process
void *getDlerrorAddr(pid_t pid) {
    void *dlErrorAddress;

    if (arch == deviceArchitecture::x86 || arch == deviceArchitecture::x86_64) {
        dlErrorAddress = getRemoteFuncAddr(pid, linkerPath, (void *) dlopen);
    } else {
        if (sdkVersion <= 23) {
            dlErrorAddress = getRemoteFuncAddr(pid, linkerPath, (void *) dlerror);
        } else {
            dlErrorAddress = getRemoteFuncAddr(pid, libDLPath, (void *) dlerror);
        }
    }

    LOGE("dlerror getRemoteFuncAddr: 0x%lx", (uintptr_t) dlErrorAddress);
    return dlErrorAddress;
}

//read data from the target process
int ptrace_readdata(pid_t pid, uint8_t *pSrcBuf, uint8_t *pDestBuf, size_t size) {
    long nReadCount = 0;
    long nRemainCount = 0;
    uint8_t *pCurSrcBuf = pSrcBuf;
    uint8_t *pCurDestBuf = pDestBuf;
    long lTmpBuf = 0;
    long i = 0;

    nReadCount = size / sizeof(long);
    nRemainCount = size % sizeof(long);

    for (i = 0; i < nReadCount; i++) {
        lTmpBuf = ptrace(PTRACE_PEEKTEXT, pid, pCurSrcBuf, 0);
        memcpy(pCurDestBuf, (char *) (&lTmpBuf), sizeof(long));
        pCurSrcBuf += sizeof(long);
        pCurDestBuf += sizeof(long);
    }

    if (nRemainCount > 0) {
        lTmpBuf = ptrace(PTRACE_PEEKTEXT, pid, pCurSrcBuf, 0);
        memcpy(pCurDestBuf, (char *) (&lTmpBuf), nRemainCount);
    }

    return 0;
}

//write data to the target process
int ptrace_writedata(pid_t pid, uint8_t *pWriteAddr, uint8_t *pWriteData, size_t size) {
    long nWriteCount = 0;
    long nRemainCount = 0;
    uint8_t *pCurSrcBuf = pWriteData;
    uint8_t *pCurDestBuf = pWriteAddr;
    long lTmpBuf = 0;
    long i = 0;

    nWriteCount = size / sizeof(long);
    nRemainCount = size % sizeof(long);

    mprotect(pWriteAddr, size, PROT_READ | PROT_WRITE | PROT_EXEC);

    for (i = 0; i < nWriteCount; i++) {
        memcpy((void *)(&lTmpBuf), pCurSrcBuf, sizeof(long));
        if (ptrace(PTRACE_POKETEXT, pid, (void *)pCurDestBuf, (void *)lTmpBuf) < 0) {
            LOGE("Write Remote Memory error, MemoryAddr: 0x%lx, error:%s\n", (uintptr_t)pCurDestBuf, strerror(errno));
            return -1;
        }
        pCurSrcBuf += sizeof(long);
        pCurDestBuf += sizeof(long);
    }

    if (nRemainCount > 0) {
        lTmpBuf = ptrace(PTRACE_PEEKTEXT, pid, pCurDestBuf, NULL);
        memcpy((void *)(&lTmpBuf), pCurSrcBuf, nRemainCount);
        if (ptrace(PTRACE_POKETEXT, pid, pCurDestBuf, lTmpBuf) < 0){
            LOGE("Write Remote Memory error, MemoryAddr: 0x%lx, err:%s\n", (uintptr_t)pCurDestBuf, strerror(errno));
            return -1;
        }
    }
    return 0;
}

//call a function on the target process
int ptrace_call(pid_t pid, uintptr_t ExecuteAddr, long *parameters, long num_params,struct pt_regs *regs) {
    #if defined(__i386__)
        regs->esp -= (num_params) * sizeof(long);
        if (0 != ptrace_writedata(pid, (uint8_t *)regs->esp, (uint8_t *)parameters,(num_params) * sizeof(long))){
            return -1;
        }

        long tmp_addr = 0x0;
        regs->esp -= sizeof(long);
        if (0 != ptrace_writedata(pid, (uint8_t *)regs->esp, (uint8_t *)&tmp_addr, sizeof(tmp_addr))){
            return -1;
        }

        regs->eip = ExecuteAddr;

        if (ptrace_setregs(pid, regs) == -1 || ptraceContinue(pid) == -1) {
            return -1;
        }

        int stat = 0;
        waitpid(pid, &stat, WUNTRACED);

        LOGI("ptrace call return value is %d", stat);
        while (stat != 0xb7f) {
            if (ptraceContinue(pid) == -1) {
                LOGE("ptrace call error");
                return -1;
            }
            waitpid(pid, &stat, WUNTRACED);
        }

        if (ptrace_getregs(pid, regs) == -1) {
            return -1;
        }
    #elif defined(__x86_64__)
       LOGE("Ptrace call x86_64");
       int num_param_registers = 6;
       if (num_params > 0)
           regs->rdi = parameters[0];
       if (num_params > 1)
           regs->rsi = parameters[1];
       if (num_params > 2)
           regs->rdx = parameters[2];
       if (num_params > 3)
           regs->rcx = parameters[3];
       if (num_params > 4)
           regs->r8 = parameters[4];
       if (num_params > 5)
           regs->r9 = parameters[5];

       if (num_param_registers < num_params){
           regs->esp -= (num_params - num_param_registers) * sizeof(long); //Allocate stack space, the direction of the stack is from high address to low address
           if (0 != ptrace_writedata(pid, (uint8_t *)regs->esp, (uint8_t *)&parameters[num_param_registers], (num_params - num_param_registers) * sizeof(long))){
               return -1;
           }
       }

       long tmp_addr = 0x0;
       regs->esp -= sizeof(long);
       if (0 != ptrace_writedata(pid, (uint8_t *)regs->esp, (uint8_t *)&tmp_addr, sizeof(tmp_addr))) {
           return -1;
       }

       regs->eip = ExecuteAddr;

       if (ptrace_setregs(pid, regs) == -1 || ptrace_continue(pid) == -1) {
           return -1;
       }

       int stat = 0;
       waitpid(pid, &stat, WUNTRACED);

       while (stat != 0xb7f){
           if (ptrace_continue(pid) == -1){
               //printf("[-] ptrace call error");
               return -1;
           }
           waitpid(pid, &stat, WUNTRACED);
       }

    #elif defined(__arm__) || defined(__aarch64__)
    #if defined(__arm__)
        int num_param_registers = 4;
    #elif defined(__aarch64__) // 64-bit real machine
        int num_param_registers = 8;
    #endif
        int i = 0;
        for (i = 0; i < num_params && i < num_param_registers; i++){
            regs->uregs[i] = parameters[i];
        }

        if (i < num_params) {
            regs->ARM_sp -= (num_params - i) * sizeof(long);
            if (ptrace_writedata(pid, (uint8_t *)(regs->ARM_sp), (uint8_t *)&parameters[i], (num_params - i) * sizeof(long)) == -1)
                return -1;
        }

        regs->ARM_pc = ExecuteAddr;
        if (regs->ARM_pc & 1){
            regs->ARM_pc &= (~1u);
            regs->ARM_cpsr |= CPSR_T_MASK;
        } else {
            regs->ARM_cpsr &= ~CPSR_T_MASK;
        }

        regs->ARM_lr = 0;

        long lr_val = 0;
        char sdk_ver[32];
        memset(sdk_ver, 0, sizeof(sdk_ver));
        __system_property_get("ro.build.version.sdk", sdk_ver);
        if (atoi(sdk_ver) <= 23) {
            lr_val = 0;
        } else { // Android 7.0
            static long start_ptr = 0;
            if (start_ptr == 0){
                start_ptr = (long)getModuleBaseAddr(pid, libcPath);
            }
            lr_val = start_ptr;
        }
        LOGE("lr_val: %ld", lr_val);
        regs->ARM_lr = lr_val;

        if (ptrace_setregs(pid, regs) == -1 || ptraceContinue(pid) == -1){
            return -1;
        }

        int stat = 0;
        waitpid(pid, &stat, WUNTRACED);

        while ((stat & 0xFF) != 0x7f){
            if (ptraceContinue(pid) == -1) {
                return -1;
            }
            waitpid(pid, &stat, WUNTRACED);
        }

        if (ptrace_getregs(pid, regs) == -1){
            return -1;
        }
    #else
        LOGE("Unsupported device");
    #endif
    return 0;
}
