//
// Created by reveny on 5/17/24.
//

#include <asm-generic/mman.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <cstring>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <xdl.h>

#include <RevMemory.hpp>
#include <NativeBridge.hpp>
#include <RemapTools.hpp>
#include <asm-generic/fcntl.h>
#include <libgen.h>
#include <android/dlext.h>

int RevMemory::Inject(pid_t pid, std::string library, bool remap) {
    // Attach to the Remote Process
    if (xptrace(PTRACE_ATTACH, pid) == -1) {
        LOGE("[-] Attach to Process %d failed", pid);
        return -1;
    }

    // Initialize Remote Process
    std::shared_ptr<RemoteProcess> process = std::make_shared<RemoteProcess>(pid);

    // Allocate Memory and write target library path to it
    uintptr_t map = process->RemoteString(library);
    LOGI("[+] Successfully wrote path to address %p", map);

    // Call DLOPEN on the allocated memory
    uintptr_t dlopen_ret = process->call((void *)dlopen, map, RTLD_GLOBAL | RTLD_NOW);
    if (dlopen_ret == 0x0) {
        LOGE("[-] DLOPEN Failed: Returned Handle %p", dlopen_ret);

        // Call DLError
        uintptr_t error = process->call((void *)dlerror);
        uintptr_t len = process->call((void *)strlen, error);

        char error_str[256];
        process->Read(error, error_str, len);
        LOGE("[-] DLOPEN Failed: %s", error_str);

        // Detach Process and return
        (void)xptrace(PTRACE_DETACH, pid);
        return -1;
    }
    LOGI("[+] DLOPEN Return value: %p", dlopen_ret);

    // Check if library needs to be remapped and remap it
    if (remap) {
        std::string name = library.substr(library.find_last_of("/\\") + 1);

        LOGI("[+] Remap option selected: Attempting to remap %s", name.c_str());
        RevMemory::RemoteRemap(process, name);
    }

    // Detach Process
    (void)xptrace(PTRACE_DETACH, pid);

    return 1;
}

int RevMemory::EmulatorInject(pid_t pid, std::string library, bool remap) {
    // Attach to the Remote Process
    if (xptrace(PTRACE_ATTACH, pid) == -1) {
        LOGE("[-] Attach to Process %d failed", pid);
        return -1;
    }

    // Initialize Remote Process
    std::shared_ptr<RemoteProcess> process = std::make_shared<RemoteProcess>(pid);

    // Read target library and get size, the problem is that opening a file remotely does
    // not work correctly, this is why we load it in our own process and then manually write it...
    int local_open = open(library.c_str(), O_RDONLY);
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

    size_t local_size = static_cast<size_t>(file_stat.st_size);
    void *local_map = mmap(nullptr, local_size, PROT_READ, MAP_SHARED, local_open, 0);
    if (local_map == nullptr) {
        LOGE("[-] Error while locally mapping file: %s", strerror(errno));
        close(local_open);
        return -1;
    }
    close(local_open);

    // Allocate the same map remotely
    void* remote_data = (void *)process->call(mmap, 0, local_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (remote_data == nullptr) {
        LOGE("[-] Error while remotely mapping library data");
        return -1;
    }

    // Manually write the data to the map
    if (!RevMemory::ProcessVirtualMemory(process->GetPID(), remote_data, local_map, local_size, true)) {
        LOGE("[-] Failed to write local map to remote map");
        return -1;
    }

    // Unmap the local space we allocated
    (void)munmap(local_map, local_size);

    int nb_result = NativeBridge::LoadNativeBridge(process, remote_data, local_size);

    // Check if library needs to be remapped and remap it
    if (remap) {
        std::string name = library.substr(library.find_last_of("/\\") + 1);

        LOGI("[+] Remap option selected: Attempting to remap %s", name.c_str());
        RevMemory::RemoteRemap(process, name);
    }

    // Detach Process
    (void)xptrace(PTRACE_DETACH, pid);

    return nb_result;
}

int RevMemory::InjectLinkerBypass(std::shared_ptr<RemoteProcess> process, std::string libraryPath, std::shared_ptr<InjectorData> data) {
    // Due to namespace restrictions we need to locally load the library with xdl and get the remote offset
    void *local_dl_handle = xdl_open("linker64", XDL_TRY_FORCE_LOAD);
    if (local_dl_handle == nullptr) {
        LOGE("[-] Failed to open libdl.so locally");
        return -1;
    }

    void *local_create_namespace = xdl_sym(local_dl_handle, "__loader_android_create_namespace", nullptr);
    if (local_create_namespace == nullptr) {
        LOGE("[-] Failed to resolve __loader_android_create_namespace locally");
        return -1;
    }

    uintptr_t remote_create_namespace = GetRemoteFunctionAddr(process->GetPID(), "libdl.so", (uintptr_t)local_create_namespace);
    uintptr_t remote_dlopen_ext = GetRemoteFunctionAddr(process->GetPID(), "libdl.so", (uintptr_t)android_dlopen_ext);
    LOGI("[+] Resolved __loader_android_create_namespace: %p", remote_create_namespace);
    LOGI("[+] Resolved android_dlopen_ext: %p", remote_dlopen_ext);

    char* dir = dirname(libraryPath.c_str());

    // Create namespace in the remote process
    uintptr_t remote_ns = process->a_call(remote_create_namespace, process->RemoteString(libraryPath), process->RemoteString(dir), nullptr, 2, nullptr, nullptr, nullptr);
    if (!remote_ns) {
        LOGE("[-] Failed to create namespace in the remote process.");
        return -1;
    }

    // Prepare android_dlextinfo in the remote process
    android_dlextinfo extinfo = {};
    extinfo.flags = ANDROID_DLEXT_USE_NAMESPACE;
    extinfo.library_namespace = reinterpret_cast<android_namespace_t*>(remote_ns);

    // Allocate memory for android_dlextinfo in the remote process
    uintptr_t remote_extinfo = process->call(malloc, sizeof(android_dlextinfo));
    if (!remote_extinfo) {
        LOGE("[-] Failed to allocate memory for android_dlextinfo in remote process.");
        return -1;
    }

    // Write the android_dlextinfo structure into the remote process memory
    process->Write(remote_extinfo, &extinfo, sizeof(android_dlextinfo));

    // Call android_dlopen_ext in the remote process with the namespace
    uintptr_t remote_library_handle = process->a_call(remote_dlopen_ext, process->RemoteString(libraryPath), RTLD_GLOBAL | RTLD_NOW, remote_extinfo);
    if (!remote_library_handle) {
        LOGE("[-] Failed to load library in remote process.");

        // Read dlerror
        uintptr_t error = process->call(dlerror);
        if (error != -1) {
            char err[256];
            process->Read(error, err, sizeof(err));
            LOGE("[-] DLOPEN EXT Error: %s", err);
        }

        return -1;
    }

    LOGI("[+] Successfully loaded library %s in remote process with handle: %p", libraryPath.c_str(), remote_library_handle);

    // Remote allocate memory for the InjectorData structure
    uintptr_t remote_data = process->call(malloc, sizeof(InjectorData));
    if (!remote_data) {
        LOGE("[-] Failed to allocate memory for InjectorData in remote process.");
        process->call(dlclose, remote_library_handle);
        return -1;
    }

    // Write the InjectorData structure into the remote process memory
    process->Write(remote_data, &data, sizeof(InjectorData));

    // Call remote dlsym to "entry"
    uintptr_t remote_entry = process->call(dlsym, remote_library_handle, process->RemoteString("entry"));
    if (!remote_entry) {
        LOGE("[-] Failed to resolve inject_entry symbol in remote process.");
        process->call(dlclose, remote_library_handle);
        return -1;
    }
    LOGI("[+] Successfully resolved entry symbol in remote process: %p", remote_entry);

    // Call the entry function in the remote process while still passing the InjectorData structure
    // TODO: Check if we need to manually copy over some data in the structure
    int proxyResult = static_cast<int>(process->a_call(remote_entry, remote_data));
    LOGI("[+] Successfully called entry function in remote process: %d", proxyResult);

    return 1;
}

// TODO: Future support for this. I already know how to do this but will add this in a future release
int RevMemory::InjectMemfdDlopen(std::shared_ptr<RemoteProcess> process, std::string libraryPath, std::shared_ptr<InjectorData> data) {
    auto generateRandomString = [](size_t min, size_t max) -> std::string {
        std::string str;
        size_t length = rand() % (max - min + 1) + min;
        for (size_t i = 0; i < length; i++) {
            str += (char) (rand() % 26 + 'a');
        }
        return str;
    };

    std::string memfd_rand = generateRandomString(5, 12);
    LOGI("[+] Random String: %s", memfd_rand.c_str());

    // String to other process
    uintptr_t memfd_name_str = process->RemoteString(memfd_rand);
    if (!memfd_name_str) {
        LOGE("[-] Failed to write memfd_name to remote process");
        return -1;
    }

    int memfd_call = process->call(syscall, __NR_memfd_create, memfd_name_str, MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (memfd_call == -1) {
        LOGE("[-] Failed to create memfd remotely");
        return -1;
    }
    LOGI("[+] Successfully created memfd: %d", memfd_call);

    auto writeFileToMemfd = [process](const char *library, int memfd) -> bool {
        // Open locally
        int localFD = open(library, O_RDONLY);
        struct stat localFileStat{};
        if (fstat(localFD, &localFileStat) == -1) {
            LOGE("[-] Failed to fstat file: errno: %d", errno);
            return false;
        }
        size_t fileSize = localFileStat.st_size;

        // Local mmap
        void* localMemory = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, localFD, 0);
        LOGI("[+] Local Memory: %p", localMemory);

        // Map the file into memory
        uintptr_t remoteMemory = process->call(mmap, nullptr, fileSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        LOGI("[+] Remote Memory: %p (%d)", remoteMemory, fileSize);

        if (remoteMemory == (uintptr_t)-1) {
            LOGE("[-] Failed to mmap remote memory: errno: %d", errno);
            process->call(close, memfd);
            return false;
        }

        // Write local memory to remote memory
        bool remote_write_result = RevMemory::ProcessVirtualMemory(process->GetPID(), (void *)remoteMemory, (void *)localMemory, fileSize, true);
        if (!remote_write_result) {
            LOGE("[-] Failed to write local memory to remote memory");
            process->call(munmap, remoteMemory, fileSize);
            process->call(close, memfd);
            return false;
        }
        LOGI("[+] Successfully wrote local memory to remote memory: %d", remote_write_result);

        // Write the content of the mapped file to the memfd
        size_t write_size = process->call(write, memfd, remoteMemory, fileSize);
        if (write_size != fileSize) {
            LOGE("[-] Failed to write file content to memfd: size written: %d", write_size);
            process->call(munmap, remoteMemory, fileSize);
            process->call(close, memfd);
            return false;
        }

        munmap(localMemory, fileSize);
        return true;
    };

    if (!writeFileToMemfd(libraryPath.c_str(), memfd_call)) {
        LOGE("[-] Failed to write file to memfd");
        return -1;
    }

    // Prepare android_dlextinfo for remote dlopen
    android_dlextinfo extinfo = {};
    extinfo.flags = ANDROID_DLEXT_USE_LIBRARY_FD;
    extinfo.library_fd = memfd_call;

    // Copy the extinfo struct to the remote process
    uintptr_t remote_extinfo = process->call(malloc, sizeof(android_dlextinfo));
    if (!process->Write(remote_extinfo, &extinfo, sizeof(android_dlextinfo))) {
        LOGE("[-] Failed to write extinfo to remote process");
        return -1;
    }
    LOGI("[+] Successfully wrote extinfo to remote process: %p", remote_extinfo);

    // Call dlopen ext
    uintptr_t dlopen_ext = process->call(android_dlopen_ext, memfd_name_str, RTLD_NOW | RTLD_GLOBAL, remote_extinfo);
    if (dlopen_ext == 0x0) {
        LOGE("[-] DLOPEN EXT Failed: Returned Handle %p", dlopen_ext);

        // Read dlerror
        uintptr_t error = process->call(dlerror);
        if (error != -1) {
            char err[256];
            process->Read(error, err, sizeof(err));
            LOGE("[-] DLOPEN EXT Error: %s", err);
        }

        return -1;
    }
    LOGI("[+] DLOPEN EXT Success: Returned Handle %p", dlopen_ext);

    // Write the InjectorData structure into the remote process memory
    uintptr_t remoteData = CopyInjectorData(process, data);
    if (!remoteData) {
        LOGE("[-] Failed to copy InjectorData to remote process");
        return -1;
    }

    // Call remote dlsym to "entry"
    uintptr_t remote_entry = process->call(dlsym, dlopen_ext, process->RemoteString("entry"));
    if (!remote_entry) {
        LOGE("[-] Failed to resolve inject_entry symbol in remote process.");
        return -1;
    }
    LOGI("[+] Successfully resolved entry symbol in remote process: %p", remote_entry);

    // Call the entry function in the remote process while still passing the InjectorData structure
    // TODO: Check if we need to manually copy over some data in the structure
    int proxyResult = static_cast<int>(process->a_call(remote_entry, remoteData));
    LOGI("[+] Successfully called entry function in remote process: %d", proxyResult);

    return 1;
}

int RevMemory::InjectProxy(pid_t pid, std::string libraryPath, std::shared_ptr<InjectorData> data) {
    // Attach to the Remote Process
    if (xptrace(PTRACE_ATTACH, pid) == -1) {
        LOGE("[-] Attach to Process %d failed", pid);
        return -1;
    }

    // Initialize Remote Process
    std::shared_ptr<RemoteProcess> process = std::make_shared<RemoteProcess>(pid);

    // Allocate Memory and write target library path to it
    uintptr_t map = process->RemoteString(libraryPath);
    LOGI("[+] Successfully wrote path to address %p", map);

    // Unmap all previous instances of the library
    if (data->getInjectZygote()) {
        LOGI("[+] Injecting into Zygote, unmap all instances of the library");
        size_t pos = libraryPath.find_last_of("/\\");
        std::string fileName = (pos == std::string::npos) ? libraryPath : libraryPath.substr(pos + 1);

        std::vector<RemapTools::MapInfo> current = RemapTools::ListModulesWithName(pid, fileName);
        for (const RemapTools::MapInfo& info : current) {
            // Call Remote munmap
            (void) process->call(munmap, (void *) info.start, info.end - info.start);
        }
        LOGI("[+] Unmapped %d instances of the library", current.size());
    }

    if (InjectMemfdDlopen(process, libraryPath, data) == -1) {
        LOGE("[-] Failed to call linker bypass remotely");

        // Detach Process and return
        (void)xptrace(PTRACE_DETACH, pid);
        return -1;
    }

    // Detach Process
    (void)xptrace(PTRACE_DETACH, pid);

    return 1;
}

uintptr_t RevMemory::CopyInjectorData(std::shared_ptr<RemoteProcess> process, std::shared_ptr<InjectorData> data) {
    RemoteInjectorData remoteData{};
    remoteData.shouldAutoLaunch = data->getShouldAutoLaunch();
    remoteData.shouldKillBeforeLaunch = data->getShouldKillBeforeLaunch();
    remoteData.injectZygote = data->getInjectZygote();
    remoteData.remapLibrary = data->getRemapLibrary();
    remoteData.useProxy = data->getUseProxy();
    remoteData.randomizeProxyName = data->getRandomizeProxyName();
    remoteData.copyToCache = data->getCopyToCache();
    remoteData.hideLibrary = data->getHideLibrary();
    remoteData.bypassNamespaceRestrictions = data->getBypassNamespaceRestrictions();

    // Remote allocate memory for the InjectorData structure
    uintptr_t remote_data = process->call(malloc, sizeof(RemoteInjectorData));
    if (!remote_data) {
        LOGE("[-] Failed to allocate memory for InjectorData in remote process.");
        return 0;
    }

    // Allocate remote strings
    remoteData.packageName = process->RemoteString(data->getPackageName());
    remoteData.launcherActivity = process->RemoteString(data->getLauncherActivity());
    remoteData.libraryPath = process->RemoteString(data->getLibraryPath());

    // Write the InjectorData structure into the remote process memory
    process->Write(remote_data, &remoteData, sizeof(InjectorData));
    return remote_data;
}

bool RevMemory::ProcessVirtualMemory(pid_t pid, void *address, void *buffer, size_t size, bool iswrite) {
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = size;
    remote[0].iov_base = address;
    remote[0].iov_len = size;

    long bytes = syscall((iswrite ? process_vm_writev_syscall : process_vm_readv_syscall), pid, local, 1, remote, 1, 0);
    LOGI("[+] ProcessVirtualMemory: %s %p -> %p (%d bytes)", (iswrite ? "Write" : "Read"), address, buffer, size);

    return ((size_t)bytes == size);
}

void RevMemory::LaunchApp(std::string activity) {
    std::string command = "am start " + std::string(activity);
    LOGI("[+] Launch Command: %s", command.c_str());

    system(command.c_str());
}

pid_t RevMemory::WaitForProcess(std::string packageName) {
    while (true) {
        int pid = FindProcessID(packageName);
        if (pid != -1) {
            // Ensure the process is fully started
            sleep(1);
            return pid;
        }
        sleep(1);
    }
}

pid_t RevMemory::FindProcessID(std::string packageName) {
    DIR* dir = opendir("/proc");
    if (packageName.empty() || dir == nullptr) {
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        int pid = atoi(entry->d_name);

        if (pid == 0) {
            continue;
        }

        char file_name[30];
        char temp_name[50];

        snprintf(file_name, 30, "/proc/%d/cmdline", pid);
        FILE *fp = fopen(file_name, "r");
        if (fp == nullptr) {
            continue;
        }

        fgets(temp_name, 50, fp);
        fclose(fp);

        if (strcmp(packageName.c_str(), temp_name) == 0) {
            return pid;
        }
    }

    return -1;
}

void RevMemory::SetSELinux(int enabled) {
    #if defined(__arm__) || defined(__aarch64__)
    std::ifstream mountsFile("/proc/mounts");
    if (!mountsFile.is_open()) {
        std::cerr << "Failed to open /proc/mounts" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(mountsFile, line)) {
        if (line.find("selinuxfs") == std::string::npos) {
            continue;
        }

        std::istringstream iss(line);
        std::string selinux_dir;
        iss >> selinux_dir >> selinux_dir;

        std::string selinux_path = selinux_dir + "/enforce";

        std::ofstream selinuxFile(selinux_path);
        if (!selinuxFile.is_open()) {
            LOGE("[-] Failed to open %s", selinux_path.c_str());
            return;
        }

        selinuxFile << (enabled ? "1" : "0");
        selinuxFile.close();
        break;
    }
    mountsFile.close();
    #elif defined(__x86_64__) || defined(__i386__)
    // Attempt to set SELinux, if it doesn't succeed, it should not
    // crash the emulator like the other code does
    if (enabled) {
        system("setenforce 1");
    } else {
        system("setenforce 0");
    }
    #endif
}

// There is probably a better way to do this but for now this will work
std::string RevMemory::GetNativeLibraryDirectory() {
    std::vector<RemapTools::MapInfo> info = RemapTools::ListModulesWithName(-1, "libRevenyInjector.so");
    for (size_t i = 0; i < info.size(); ++i) {
        // Remove libRevenyInjector.so to get path
        std::string pathStr = info[i].path;
        if (!pathStr.empty()) {
            pathStr.resize(pathStr.size() - strlen("libRevenyInjector.so"));
            return pathStr;
        }
    }

    return {};
}

uintptr_t RevMemory::GetModuleBase(pid_t pid, std::string moduleName) {
    std::vector<RemapTools::MapInfo> info = RemapTools::ListModulesWithName(pid, moduleName);
    for (size_t i = 0; i < info.size(); ++i) {
        if (info[i].perms & PROT_EXEC) {
            return info[i].start;
        }
    }

    return {};
}

void RevMemory::RemoteRemap(std::shared_ptr<RemoteProcess> process, std::string libraryName) {
    std::vector<RemapTools::MapInfo> current = RemapTools::ListModulesWithName(process->GetPID(), libraryName);
    LOGI("[+] Remote Remap: Found %d Segments to remap", current.size());

    for (RemapTools::MapInfo info : current) {
        void *address = (void *)info.start;
        size_t size = info.end - info.start;

        // Remote Allocate space for the library
        uintptr_t map = process->call(mmap, 0, size, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (map == 0x0) {
            LOGE("[-] Remote Remap: Failed to allocate memory: %s", strerror(errno));
            continue;
        }
        LOGI("[+] Remote Remap: Allocated Space for Library at address %p", map);

        if ((info.perms & PROT_READ) == 0) {
            LOGI("[+] Remote Remap: Removing protection: %s", info.path.c_str());

            // Call Remote mprotect (return ignored)
            (void)process->call(mprotect, address, size, PROT_READ);
        }

        // Call Remote memcpy
        (void)process->call(memcpy, map, address, size);

        // Call Remote mremap
        (void)process->call(mremap, map, size, size, MREMAP_MAYMOVE | MREMAP_FIXED, info.start);

        // Call Remote mprotect to reapply the protection
        (void)process->call(mprotect, (void *)info.start, size, info.perms);
    }
}

std::string RevMemory::GetRemoteModuleName(pid_t pid, uintptr_t addr) {
    std::vector<RemapTools::MapInfo> info = RemapTools::ListModulesNew(pid);
    for (size_t i = 0; i < info.size(); ++i) {
        if (addr >= info[i].start && addr <= info[i].end) {
            return info[i].path;
        }
    }

    return {};
}

uintptr_t RevMemory::GetRemoteFunctionAddr(pid_t pid, std::string moduleName, uintptr_t localFuncAddr) {
    uintptr_t localModuleAddr, remoteModuleAddr;
    
    // Get the starting address of a module locally
    localModuleAddr = GetModuleBase(-1, moduleName);
    
    // Get the starting address of a module in a remote process by PID
    remoteModuleAddr = GetModuleBase(pid, moduleName);
    
    // Calculate the offset of the function (e.g., mmap) in the module
    // and add it to the remote module base address
    return localFuncAddr - localModuleAddr + remoteModuleAddr;
}