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
    struct MapInfo {
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

    std::vector<MapInfo> ListModulesWithName(pid_t pid, std::string name);
    std::vector<MapInfo> ListModulesNew(pid_t pid);

    void RemapLibrary(std::string name);
}