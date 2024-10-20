//
// Created by reveny on 5/30/24.
//
#include <Include/RemapTools.hpp>
#include <Include/Logger.hpp>

std::vector<RemapTools::MapInfo> RemapTools::ListModulesWithName(pid_t pid, std::string name) {
    std::vector<MapInfo> returnVal;

    std::vector<MapInfo> info = ListModulesNew(pid);
    for (size_t i = 0; i < info.size(); ++i) {
        if (info[i].path.find(name) != std::string::npos) {
            returnVal.push_back(info[i]);
        }
    }

    return returnVal;
}

// https://github.com/LSPosed/LSPlt/blob/a674793be6bc060b6d695c0cb481e8262c763885/lsplt/src/main/jni/lsplt.cc#L245
std::vector<RemapTools::MapInfo> RemapTools::ListModulesNew(pid_t pid) {
    constexpr static auto kPermLength = 5;
    constexpr static auto kMapEntry = 7;
    std::vector<MapInfo> info;

    std::string path = "/proc/self/maps";
    if (pid != -1) {
        path = "/proc/" + std::to_string(pid) + "/maps";
    }

    auto maps = std::unique_ptr<FILE, decltype(&fclose)>{fopen(path.c_str(), "r"), &fclose};
    if (maps) {
        char *line = nullptr;
        size_t len = 0;
        ssize_t read;

        while ((read = getline(&line, &len, maps.get())) > 0) {
            line[read - 1] = '\0';
            uintptr_t start = 0;
            uintptr_t end = 0;
            uintptr_t off = 0;
            ino_t inode = 0;

            unsigned int dev_major = 0;
            unsigned int dev_minor = 0;
            std::array<char, kPermLength> perm{'\0'};

            int path_off;
            if (sscanf(line, "%" PRIxPTR "-%" PRIxPTR " %4s %" PRIxPTR " %x:%x %lu %n%*s", &start, &end, perm.data(), &off, &dev_major, &dev_minor, &inode, &path_off) != kMapEntry) {
                continue;
            }
            while (path_off < read && isspace(line[path_off])) path_off++;

            auto &ref = info.emplace_back(MapInfo{line, start, end, 0, perm[3] == 'p', off, static_cast<dev_t>(makedev(dev_major, dev_minor)), inode, line + path_off});
            if (perm[0] == 'r') ref.perms |= PROT_READ;
            if (perm[1] == 'w') ref.perms |= PROT_WRITE;
            if (perm[2] == 'x') ref.perms |= PROT_EXEC;
        }
        free(line);
    }
    return info;
}

void RemapTools::RemapLibrary(std::string name) {
    std::vector<MapInfo> maps = ListModulesWithName(-1, name);

    for (MapInfo info : maps) {
        void *address = (void *)info.start;
        size_t size = info.end - info.start;
        void *map = mmap(0, size, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        if ((info.perms & PROT_READ) == 0) {
            LOGI("Removing protection: %s", info.path.c_str());
            mprotect(address, size, PROT_READ);
        }

        if (map == nullptr) {
            LOGE("Failed to Allocate Memory: %s", strerror(errno));
            return;
        }
        LOGI("Allocated at address %p with size of %zu", map, size);

        // Copy to new location
        std::memmove(map, address, size);
        mremap(map, size, size, MREMAP_MAYMOVE | MREMAP_FIXED, info.start);

        // Reapply protection
        mprotect((void *)info.start, size, info.perms);
    }
}