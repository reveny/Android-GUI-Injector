//
// Created by reveny on 10/07/2023.
// Inspired by riru and zygisk
// This is a single header library
//

#include <c++/v1/cstdint>
#include <link.h>
#include <sys/mman.h>
#include <c++/v1/vector>
#include <c++/v1/string>
#include <sys/sysmacros.h>
#include <inttypes.h>
#include <stdint.h>

namespace RemapTools {
    struct ProcMapInfo {
        u_long start;
        u_long end;
        u_long offset;
        uint8_t perms;
        ino_t inode;
        char* dev;
        char* path;
    };

    struct MapInfo
    {
        std::string line;
        uintptr_t start;
        uintptr_t end;
        uint8_t perms;
        bool is_private;
        uintptr_t offset;
        dev_t dev;
        ino_t inode;
        std::string path;
    };

    inline std::vector<ProcMapInfo> ListModulesWithName(std::string name)
    {
        std::vector<ProcMapInfo> returnVal;

        char buffer[512];
        FILE *fp = fopen("/proc/self/maps", "re");
        if (fp != nullptr)
        {
            while (fgets(buffer, sizeof(buffer), fp))
            {
                if (strstr(buffer, name.c_str()))
                {
                    RemapTools::ProcMapInfo info{};
                    char perms[10], path[255], dev[25];

                    #if defined(__aarch64__) || defined(__x86_64__)
                    sscanf(buffer, "%lx-%lx %s %lx %s %lu %s", &info.start, &info.end, perms, &info.offset, dev, &info.inode, path);
                    #elif defined(__arm__) || defined(__i386__)
                    sscanf(buffer, "%x-%x %s %x %s %l %s", &info.start, &info.end, perms, &info.offset, dev, (int*)&info.inode, path);
                    #endif

                    // Process Perms
                    if (strchr(perms, 'r')) info.perms |= PROT_READ;
                    if (strchr(perms, 'w')) info.perms |= PROT_WRITE;
                    if (strchr(perms, 'x')) info.perms |= PROT_EXEC;
                    if (strchr(perms, 'r')) info.perms |= PROT_READ;

                    // Set all other information
                    info.dev = dev;
                    info.path = path;

                    returnVal.push_back(info);
                }
            }
        }

        return returnVal;
    }

    inline std::vector<ProcMapInfo> ListModules() {
        std::vector<ProcMapInfo> returnVal;

        char buffer[512];
        FILE *fp = fopen("/proc/self/maps", "re");
        if (fp != nullptr) {
            while (fgets(buffer, sizeof(buffer), fp)) {
                ProcMapInfo info{};
                char perms[10] = {0};
                char path[255] = {0}; // Initialize path to empty
                char dev[25] = {0};

                int numRead = sscanf(buffer, "%lx-%lx %s %ld %s %ld %s", &info.start, &info.end, perms, &info.offset, dev, &info.inode, path);

                //Process Perms
                if (strchr(perms, 'r')) info.perms |= PROT_READ;
                if (strchr(perms, 'w')) info.perms |= PROT_WRITE;
                if (strchr(perms, 'x')) info.perms |= PROT_EXEC;

                //Set all other information
                info.dev = dev;
                numRead == 7 ? info.path = path : info.path = "";

                if (strchr(perms, 'x'))
                {
                    LOGE("A: %s [%s]", buffer, path);
                }

                returnVal.push_back(info);
            }
        }
        return returnVal;
    }

    // https://github.com/LSPosed/LSPlt/blob/a674793be6bc060b6d695c0cb481e8262c763885/lsplt/src/main/jni/lsplt.cc#L245
    static std::vector<MapInfo> ListModulesNew()
    {
        constexpr static auto kPermLength = 5;
        constexpr static auto kMapEntry = 7;
        std::vector<MapInfo> info;
        auto maps = std::unique_ptr<FILE, decltype(&fclose)>{fopen("/proc/self/maps", "r"), &fclose};
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

    inline void RemapLibrary(std::string name) {
        std::vector<ProcMapInfo> maps = ListModulesWithName(name);

        for (ProcMapInfo info : maps) {
            void *address = (void *)info.start;
            size_t size = info.end - info.start;
            void *map = mmap(0, size, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

            if ((info.perms & PROT_READ) == 0) {
                LOGI("Removing protection: %s", info.path);
                mprotect(address, size, PROT_READ);
            }

            if (map == nullptr) {
                LOGE("Failed to Allocate Memory: %s", strerror(errno));
            }
            LOGI("Allocated at address %p with size of %zu", map, size);

            //Copy to new location
            std::memmove(map, address, size);
            mremap(map, size, size, MREMAP_MAYMOVE | MREMAP_FIXED, info.start);

            //Reapply protection
            mprotect((void *)info.start, size, info.perms);
        }
    }
}