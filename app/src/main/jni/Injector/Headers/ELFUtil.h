//
// Created by reveny on 21/10/2023.
//

#ifndef DLL_INJECTOR_V2_ELFUTIL_H
#define DLL_INJECTOR_V2_ELFUTIL_H

#include <elf.h>

namespace ELFParser {
    enum class MachineType : uint16_t {
        ELF_EM_NONE = EM_NONE,       // No machine
        ELF_EM_386 = EM_386,         // Intel 80386
        ELF_EM_ARM = EM_ARM,         // ARM
        ELF_EM_X86_64 = EM_X86_64,   // x86_64
        ELF_EM_AARCH64 = EM_AARCH64  // ARM64
    };

    static MachineType getMachineType(const char* filePath) {
        FILE *elfFile = fopen(filePath, "rb");
        if (!elfFile) {
            throw std::runtime_error("Failed to open ELF file.");
        }

        Elf32_Ehdr elfHeader;
        fread(&elfHeader, sizeof(Elf32_Ehdr), 1, elfFile);
        fclose(elfFile);

        return static_cast<MachineType>(elfHeader.e_machine);
    }
}

#endif
