#include <asm-generic/mman.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Headers/RevMemory.h"
#include "Headers/NativeBridge.h"

int RevMemory::Inject(pid_t pid, const char* library, bool remap) {
    // Attach to the Remote Process
    if (xptrace(PTRACE_ATTACH, pid) == -1) {
        LOGE("[-] Attach to Process %d failed", pid);
        return -1;
    }

    // Initialize Remote Process
    RemoteProcess process(pid);

    // Allocate Memory and write target library path to it
    uintptr_t map = process.call(malloc, 256);
    if (map == -1) {
        LOGE("[-] Failed to allocate memory for path");

        // Detach Process and return
        (void)xptrace(PTRACE_DETACH, pid);
        return -1;
    }

    bool write = process.write(map, library, strlen(library));
    if (!write) {
        LOGE("[-] Failed to write path %s to address %p", library, map);

        // Detach Process and return
        (void)xptrace(PTRACE_DETACH, pid);
        return -1;
    }
    LOGI("[+] Successfully wrote path to address %p", map);

    // Call DLOPEN on the allocated memory
    uintptr_t dlopen_ret = process.call((void *)dlopen, map, RTLD_GLOBAL | RTLD_NOW);
    if (dlopen_ret == 0x0) {
        LOGE("[-] DLOPEN Failed: Returned Handle %p", dlopen_ret);

        // Detach Process and return
        (void)xptrace(PTRACE_DETACH, pid);
        return -1;
    }
    LOGI("[+] DLOPEN Return value: %p", dlopen_ret);

    // Check if library needs to be remapped and remap it
    if (remap) {
        std::string base_path = std::string(library);
        const char* name = base_path.substr(base_path.find_last_of("/\\") + 1).c_str();

        LOGI("[+] Remap option selected: Attempting to remap %s", name);
        RevMemory::remote_remap(process, name);
    }

    // Detach Process
    (void)xptrace(PTRACE_DETACH, pid);

    return 1;
}

int RevMemory::EmulatorInject(pid_t pid, const char* library, bool remap) {
    // Attach to the Remote Process
    if (xptrace(PTRACE_ATTACH, pid) == -1) {
        LOGE("[-] Attach to Process %d failed", pid);
        return -1;
    }

    // Initialize Remote Process
    RemoteProcess process(pid);

    // Read target library and get size, the problem is that opening a file remotely does
    // not work correctly, this is why we load it in our own process and then manually write it...
    int local_open = open(library, O_RDONLY);
    if (local_open == -1) {
        LOGE("[-] Failed to locally open target library, does the file exist?");
        return -1;
    }

    struct stat file_stat{};
    if (fstat(local_open, &file_stat) == -1) {
        LOGE("[-] Error reading local file size");
        close(local_open);
        return -1;
    }

    size_t local_size = file_stat.st_size;
    void *local_map = mmap(nullptr, local_size, PROT_READ, MAP_SHARED, local_open, 0);
    if (local_map == nullptr) {
        LOGE("[-] Error while locally mapping file: %s", strerror(errno));
        close(local_open);
        return -1;
    }
    close(local_open);

    // Allocate the same map remotely
    void* remote_data = (void *)process.call(mmap, 0, local_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (remote_data == nullptr) {
        LOGE("[-] Error while remotely mapping library data");
        return -1;
    }

    // Manually write the data to the map
    if (!RevMemory::process_virtual_memory(process.get_pid(), remote_data, local_map, local_size, true)) {
        LOGE("[-] Failed to write local map to remote map");
        return -1;
    }

    // Unmap the local space we allocated
    (void)munmap(local_map, local_size);

    int nb_result = NativeBridge::LoadNativeBridge(process, remote_data, local_size);

    // Check if library needs to be remapped and remap it
    if (remap) {
        std::string base_path = std::string(library);
        const char* name = base_path.substr(base_path.find_last_of("/\\") + 1).c_str();

        LOGI("[+] Remap option selected: Attempting to remap %s", name);
        RevMemory::remote_remap(process, name);
    }

    // Detach Process
    (void)xptrace(PTRACE_DETACH, pid);

    return nb_result;
}

bool RevMemory::process_virtual_memory(pid_t pid, void *address, void *buffer, size_t size, bool iswrite) {
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = size;
    remote[0].iov_base = address;
    remote[0].iov_len = size;

    ssize_t bytes = syscall((iswrite ? process_vm_writev_syscall : process_vm_readv_syscall), pid, local, 1, remote, 1, 0);
    return bytes == size;
}

void RevMemory::launch_app(const char *activity) {
    std::string command = "am start " + std::string(activity);
    LOGI("[+] Launch Command: %s", command.c_str());

    system(command.c_str());
}

int RevMemory::find_process_id(const char *packageName) {
    DIR* dir = opendir("/proc");

    if (packageName == nullptr || dir == nullptr) {
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        int pid = atoi(entry->d_name);

        if (pid != 0) {
            char file_name[30];
            char temp_name[50];

            snprintf(file_name, 30, "/proc/%d/cmdline", pid);
            FILE *fp = fopen(file_name, "r");

            if (fp != nullptr) {
                fgets(temp_name, 50, fp);
                fclose(fp);

                if (strcmp(packageName, temp_name) == 0) {
                    return pid;
                }
            }
        }
    }
    return -1;
}

void RevMemory::set_selinux(int enabled) {
    char line[1024];
    FILE* fp = fopen("/proc/mounts", "r");
    while(fgets(line, 1024, fp)) {
        if (strstr(line, "selinuxfs")) {
            strtok(line, " ");
            char* selinux_dir = strtok(nullptr, " ");
            char* selinux_path = strcat(selinux_dir, "/enforce");

            FILE* fp_selinux = fopen(selinux_path, "w");
            char buf[2];
            if (enabled) {
                strcpy(buf, "1");
            } else {
                strcpy(buf, "0");
            }

            fwrite(buf, strlen(buf), 1, fp_selinux);
            fclose(fp_selinux);
            break;
        }
    }
    fclose(fp);
}

void* RevMemory::get_module_base_addr(pid_t pid, const char* moduleName) {
    long moduleBaseAddr = 0;
    char szFileName[50] = {0};
    char szMapFileLine[1024] = {0};

    if (pid < 0) {
        snprintf(szFileName, sizeof(szFileName), "/proc/self/maps");
    } else {
        snprintf(szFileName, sizeof(szFileName), "/proc/%d/maps", pid);
    }

    FILE *fp = fopen(szFileName, "r");
    if (fp != nullptr) {
        while (fgets(szMapFileLine, sizeof(szMapFileLine), fp)) {
            if (strstr(szMapFileLine, moduleName)) {
                char* addr = strtok(szMapFileLine, "-");
                moduleBaseAddr = strtoul(addr, nullptr, 16);

                if (moduleBaseAddr == 0x8000)
                    moduleBaseAddr = 0;

                break;
            }
        }

        fclose(fp);
    }

    return (void*)moduleBaseAddr;
}

std::vector<RevMemory::ProcMapInfo> RevMemory::list_modules(pid_t pid, const char* library) {
    std::vector<ProcMapInfo> returnVal;

    char buffer[512];
    char szFileName[50] = {0};

    if (pid < 0) {
        snprintf(szFileName, sizeof(szFileName), "/proc/self/maps");
    } else {
        snprintf(szFileName, sizeof(szFileName), "/proc/%d/maps", pid);
    }

    FILE *fp = fopen(szFileName, "r");
    if (fp != nullptr) {
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strstr(buffer, library)) {
                ProcMapInfo info{};
                char perms[10];
                char path[255];
                char dev[25];

                sscanf(buffer, "%lx-%lx %s %ld %s %ld %s", &info.start, &info.end, perms, &info.offset, dev, &info.inode, path);

                //Process Perms
                if (strchr(perms, 'r')) info.perms |= PROT_READ;
                if (strchr(perms, 'w')) info.perms |= PROT_WRITE;
                if (strchr(perms, 'x')) info.perms |= PROT_EXEC;
                if (strchr(perms, 'r')) info.perms |= PROT_READ;

                //Set all other information
                info.dev = dev;
                info.path = path;

                returnVal.push_back(info);
            }
        }
    }
    return returnVal;
}

void RevMemory::remote_remap(RemoteProcess process, const char* library_name) {
    std::vector<ProcMapInfo> current = list_modules(process.get_pid(), library_name);
    LOGI("[+] Remote Remap: Found %d Segments to remap", current.size());

    for (ProcMapInfo info : current) {
        void *address = (void *)info.start;
        size_t size = info.end - info.start;

        // Remote Allocate space for the library
        uintptr_t map = process.call(mmap, 0, size, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (map == 0x0) {
            LOGE("[-] Remote Remap: Failed to allocate memory: %s", strerror(errno));
            continue;
        }
        LOGI("[+] Remote Remap: Allocated Space for Library at address %p", map);

        if ((info.perms & PROT_READ) == 0) {
            LOGI("[+] Remote Remap: Removing protection: %s", info.path);

            // Call Remote mprotect (return ignored)
            (void)process.call(mprotect, address, size, PROT_READ);
        }

        // Call Remote memcpy
        (void)process.call(memcpy, map, address, size);

        // Call Remote mremap
        (void)process.call(mremap, map, size, size, MREMAP_MAYMOVE | MREMAP_FIXED, info.start);

        // Call Remote mprotect to reapply the protection
        (void)process.call(mprotect, (void *)info.start, size, info.perms);
    }
}

const char *RevMemory::get_remote_module_name(pid_t pid, uintptr_t addr) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/proc/%d/maps", pid);

    FILE* mapsFile = fopen(filepath, "r");
    if (!mapsFile) {
        LOGE("[-] Failed to Open Path %s", filepath);
        return "";
    }

    char line[1024];
    while (fgets(line, sizeof(line), mapsFile)) {
        uintptr_t startAddr, endAddr;
        sscanf(line, "%lx-%lx", &startAddr, &endAddr);

        if (addr >= startAddr && addr <= endAddr) {
            char* libPath = strchr(line, '/');
            if (libPath) {
                // Remove newline character if it exists
                char* newline = strchr(libPath, '\n');
                if (newline) {
                    *newline = '\0';
                }
                return strdup(libPath);
            }
        }
    }

    fclose(mapsFile);
    return "";
}

void* RevMemory::get_remote_func_addr(pid_t pid, const char* moduleName, void* localFuncAddr) {
    void* localModuleAddr, *remoteModuleAddr, *remoteFuncAddr;
    
    // Get the starting address of a module locally
    localModuleAddr = get_module_base_addr(-1, moduleName);
    
    // Get the starting address of a module in a remote process by PID
    remoteModuleAddr = get_module_base_addr(pid, moduleName);
    
    // Calculate the offset of the function (e.g., mmap) in the module
    // and add it to the remote module base address
    remoteFuncAddr = (void*)((uintptr_t)localFuncAddr - (uintptr_t)localModuleAddr + (uintptr_t)remoteModuleAddr);

    return remoteFuncAddr;
}