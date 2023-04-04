#include <asm/ptrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <elf.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/unistd.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/system_properties.h>
#include <vector>
#include "Logger.h"

enum deviceArchitecture {
    armeabi_v7a,
    arm64_v8a,
    x86,
    x86_64,
    Unknown,
};

//System libs
const char *libcPath = "";
const char *linkerPath = "";
const char *libDLPath = "";
deviceArchitecture arch;
int sdkVersion = 0;

void setupSystemLibs() {
    char _sdkVersion[32];
    char sysArchitecture[32];
    __system_property_get("ro.build.version.sdk", _sdkVersion);
    __system_property_get("ro.product.cpu.abi", sysArchitecture);
    sdkVersion = atoi(_sdkVersion);

    if (strcmp(sysArchitecture, "x86_64") == 0) {
        arch = deviceArchitecture::x86_64;
        libcPath = "/system/lib64/libc.so";
        linkerPath = "/system/bin/linker64";
        libDLPath = "/system/lib64/libdl.so";
    } else if (strcmp(sysArchitecture, "x86") == 0) {
        arch = deviceArchitecture::x86;
        libcPath = "/system/lib/libc.so";
        linkerPath = "/system/bin/linker";
        libDLPath = "/system/lib/arm/nb/libdl.so";
    } else if (strcmp(sysArchitecture, "arm64-v8a") == 0) {
        arch = deviceArchitecture::arm64_v8a;
        if (sdkVersion >= 29) { //Android 10
            libcPath = "/apex/com.android.runtime/lib64/bionic/libc.so";
            linkerPath = "/apex/com.android.runtime/bin/linker64";
            libDLPath = "/apex/com.android.runtime/lib64/bionic/libdl.so";
        } else {
            libcPath = "/system/lib64/libc.so";
            linkerPath = "/system/bin/linker64";
            libDLPath = "/system/lib64/libdl.so";
        }
    } else if (strcmp(sysArchitecture, "armeabi-v7a") == 0) {
        arch = deviceArchitecture::armeabi_v7a;
    } else {
        arch = deviceArchitecture::Unknown;
        LOGE("Device not supported");
    }
}

int isSELinuxEnabled() {
    bool result = 0;
    FILE* fp = fopen("/proc/filesystems", "r");
    char line[100];
    while(fgets(line, 100, fp)) {
        if (strstr(line, "selinuxfs")) {
            result = 1;
        }
    }
    fclose(fp);
    return result;
}

void disableSELinux() {
    char line[1024];
    FILE* fp = fopen("/proc/mounts", "r");
    while(fgets(line, 1024, fp)) {
        if (strstr(line, "selinuxfs")) {
            strtok(line, " ");
            char* selinux_dir = strtok(NULL, " ");
            char* selinux_path = strcat(selinux_dir, "/enforce");

            FILE* fp_selinux = fopen(selinux_path, "w");
            char buf[2] = "0"; //0 = Permissive
            fwrite(buf, strlen(buf), 1, fp_selinux);
            fclose(fp_selinux);
            break;
        }
    }
    fclose(fp);
}

void enableSELinux() {
    char line[1024];
    FILE* fp = fopen("/proc/mounts", "r");
    while(fgets(line, 1024, fp)) {
        if (strstr(line, "selinuxfs")) {
            strtok(line, " ");
            char* selinux_dir = strtok(NULL, " ");
            char* selinux_path = strcat(selinux_dir, "/enforce");

            FILE* fp_selinux = fopen(selinux_path, "w");
            char buf[2] = "1"; //0 = Enforcing
            fwrite(buf, strlen(buf), 1, fp_selinux);
            fclose(fp_selinux);
            break;
        }
    }
    fclose(fp);
}

pid_t getPID(const char* process_name) {
    if (process_name == nullptr) {
        return -1;
    }
    DIR* dir = opendir("/proc");
    if (dir == nullptr) {
        return -1;
    }
    struct dirent* entry;
    while((entry = readdir(dir)) != NULL) {
        size_t pid = atoi(entry->d_name);
        if (pid != 0) {
            char file_name[30];
            snprintf(file_name, 30, "/proc/%zu/cmdline", pid);
            FILE *fp = fopen(file_name, "r");
            char temp_name[50];
            if (fp != nullptr) {
                fgets(temp_name, 50, fp);
                fclose(fp);
                if (strcmp(process_name, temp_name) == 0) {
                    return pid;
                }
            }
        }
    }
    return -1;
}

void launchApp(char *appLaunchActivity) {
    //Test on termux:
    //am start packagename/launchActivity
    char start_cmd[1024] = "am start ";

    strcat(start_cmd, appLaunchActivity);
    LOGI("%s", start_cmd);

    system(start_cmd);
}

//Search the base address of the module in the process
void *getModuleBaseAddr(pid_t pid, const char *moduleName) {
    long moduleBaseAddr = 0;
    char mapsPath[50] = {0};
    char szMapFileLine[1024] = {0};

    snprintf(mapsPath, sizeof(mapsPath), "/proc/%d/maps", pid);
    FILE *fp = fopen(mapsPath, "r");

    if (fp != nullptr) {
        while (fgets(szMapFileLine, sizeof(szMapFileLine), fp)) {
            if (strstr(szMapFileLine, moduleName)) {
                char *address = strtok(szMapFileLine, "-");
                moduleBaseAddr = strtoul(address, nullptr, 16);

                if (moduleBaseAddr == 0x8000) {
                    moduleBaseAddr = 0;
                }

                break;
            }
        }
        fclose(fp);
    }

    return (void *)moduleBaseAddr;
}

//Get the address of the function in the module loaded by the remote process and this process
void *getRemoteFuncAddr(pid_t pid, const char *moduleName, void *localFuncAddr) {
    void *localModuleAddr = getModuleBaseAddr(getpid(), moduleName); //Starting address of the module in my process
    void *remoteModuleAddr = getModuleBaseAddr(pid, moduleName); //Starting address of module loaded in other process
    void *remoteFuncAddr = (void *)((uintptr_t)localFuncAddr - (uintptr_t)localModuleAddr + (uintptr_t)remoteModuleAddr); //Address of the function in the other process

    LOGE("getRemoteFunctionAddr path: %s, localModuleAddr: 0x%lX, remoteModuleAddr: 0x%lX, localModuleAddr: 0x%lX, remoteFuncAddr: 0x%lX", moduleName, localModuleAddr, remoteModuleAddr, localModuleAddr, remoteFuncAddr);
    return remoteFuncAddr;
}
