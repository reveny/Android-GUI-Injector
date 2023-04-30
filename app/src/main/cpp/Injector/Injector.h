#include "PtraceUtils.h"

int injectRemoteProcess();

const char *pkgName = "";
const char *appLaunchActivity = "";
const char *libraryPath = "";
const char *functionName = "";
bool shouldAutoLaunch = true;

pid_t pid = 0;

int initInject() {
    //Setup Injection
    setupSystemLibs();
    if (isSELinuxEnabled() == 1) {
        disableSELinux();
    }

    if (shouldAutoLaunch) {
        launchApp((char *) appLaunchActivity);
    }
    sleep(4); //Waiting for the process to start

    pid = getPID((char *)pkgName);
    if (pid == -1) {
        LOGE("getPID Failed");
    }

    int res = injectRemoteProcess();
    LOGW("Inject Result: %d", res);

    //Restore SELinux flag
    enableSELinux();

    return res;
}

struct pt_regs currentRegs, originalRegs;
int callRemoteMmap() {
    long parameters[1];

    void *mmapAddr = getRemoteFuncAddr(pid, libcPath, (void *)malloc);
    LOGI("Mmap Function Address: 0x%lx", mmapAddr);

    //void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offsize);
    //parameters[0] = 0; //Not needed
    //parameters[1] = 0x3000;
    //parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;
    //parameters[3] = MAP_ANONYMOUS | MAP_PRIVATE;
    //parameters[4] = 0; //Not needed
    //parameters[5] = 0; //Not needed

    //This hopefully fixes I/O Error issues
    //void* malloc(size_t size)
    parameters[0] = 256; //Size

    //Call the mmap function of the target process
    if (ptrace_call(pid, (uintptr_t)mmapAddr, parameters, 1, &currentRegs)) {
        return -1;
    }
    return 0;
}

int callRemoteDlopen(void *remoteMmapAddr) {
    long parameters[2];

    //Return value of dlopen is the start address of the loaded module
    //void *dlopen(const char *filename, int flag);
    parameters[0] = (uintptr_t) remoteMmapAddr;
    parameters[1] = RTLD_NOW | RTLD_GLOBAL;

    void *dlopen_addr = getDlOpenAddr(pid);
    void *dlErrorAddr = getDlerrorAddr(pid);
    LOGI("dlopen getRemoteFuncAddr: 0x%lx", dlopen_addr);

    //Calls dlopen which loads the lib
    if (ptrace_call(pid, (uintptr_t) dlopen_addr, parameters, 2, &currentRegs) == -1) {
        LOGE("Call dlopen Failed");
        return -1;
    }

    void *remoteModuleAddr = (void *)ptrace_getret(&currentRegs);
    LOGI("ptrace_call dlopen success, Remote module Address: 0x%lx", (long)remoteModuleAddr);

    //dlopen error
    if ((long) remoteModuleAddr == 0x0) {
        LOGE("dlopen error");
        if (ptrace_call(pid, (uintptr_t) dlErrorAddr, parameters, 0, &currentRegs) == -1) {
            LOGE("Call dlerror failed");
            return -1;
        }
        char *error = (char *) ptrace_getret(&currentRegs);
        char localErrorInfo[1024] = {0};
        ptrace_readdata(pid, (uint8_t *) error, (uint8_t *) localErrorInfo, 1024);
        LOGE("dlopen error: %s\n", localErrorInfo);
        return -1;
    }
    return 0;
}

int callRemoteFunction(void *remoteMapMemoryAddr) {
    long parameters[2];
    if (ptrace_writedata(pid, (uint8_t *) remoteMapMemoryAddr + strlen(libraryPath) + 2,(uint8_t *) functionName, strlen(functionName) + 1) == -1) {
        LOGE("Write functionName:%s to RemoteProcess error", functionName);
        return -1;
    }

    void *dlsym_addr = getDlsymAddr(pid);
    void *remoteModuleAddr = (void *)ptrace_getret(&currentRegs);

    parameters[0] = (uintptr_t) remoteModuleAddr;
    parameters[1] = (uintptr_t) ((uint8_t *) remoteMapMemoryAddr + strlen(libraryPath) + 2);

    if (ptrace_call(pid, (uintptr_t) dlsym_addr, parameters, 2, &currentRegs) == -1) {
        LOGE("Call Remote dlsym Func Failed");
        return -1;
    }
    void *remoteModuleFuncAddr = (void *) ptrace_getret(&currentRegs);
    LOGI("ptrace_call dlsym success, Remote Process ModuleFunc Addr:0x%lx", remoteModuleFuncAddr);

    if (ptrace_call(pid, (uintptr_t) remoteModuleFuncAddr, parameters, 0,&currentRegs) == -1) {
        LOGE("Call Remote injected Func Failed");
        return -1;
    }
    return 0;
}

int injectRemoteProcess() {
    //Instead of directly returning we use a value so if something fails we still detach from the process
    int returnValue = 0;

    //Attach to the target proc
    if (ptraceAttach(pid) != 0) {
        return -1;
    }

    if (ptrace_getregs(pid, &currentRegs) != 0) {
        LOGE("Ptrace getregs failed");
        return -1;
    }

    //Backup Original Register
    memcpy(&originalRegs, &currentRegs, sizeof(currentRegs));

    if (callRemoteMmap() == -1) {
        LOGE("Call Remote mmap Func Failed: %s", strerror(errno));
        returnValue = -1;
    }
    LOGI("ptrace_call mmap success. return value=%lX, pc=%lX", ptrace_getret(&currentRegs), ptrace_getpc(&currentRegs));

    // Return value is the starting address of the memory map
    void *remoteMapMemoryAddr = (void *)ptrace_getret(&currentRegs);
    LOGI("Remote Process Map Address: 0x%lx", remoteMapMemoryAddr);

    //Params:            pid,             start addr,                      content,          size
    if (ptrace_writedata(pid, (uint8_t *) remoteMapMemoryAddr, (uint8_t *) libraryPath, strlen(libraryPath) + 1) == -1) {
        LOGE("writing %s to process failed", libraryPath);
        returnValue = -1;
    }

    if (callRemoteDlopen(remoteMapMemoryAddr) == -1) {
        LOGE("Call dlopen Failed");
        returnValue = -1;
    }

    //Check if we even want to call any function
    if (strlen(functionName) != 0) {
        if (callRemoteFunction(remoteMapMemoryAddr) == -1) {
            LOGE("Write FunctionName:%s to RemoteProcess error", functionName);
            returnValue = -1;
        }
    } else {
        LOGI("Function Name is empty, skipping call");
    }

    if (ptrace_setregs(pid, &originalRegs) == -1) {
        LOGE("Could not recover reges");
        returnValue = -1;
    }

    ptrace_getregs(pid, &currentRegs);
    if (memcmp(&originalRegs, &currentRegs, sizeof(currentRegs)) != 0) {
        LOGE("Set Regs Error");
        returnValue = -1;
    }

    ptraceDetach(pid);

    return returnValue;
}