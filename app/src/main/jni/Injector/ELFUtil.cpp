//
// Created by reveny on 5/17/24.
//

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <ELFUtil.hpp>
#include <unistd.h>
#include <sys/stat.h>

ELFParser::MachineType ELFParser::GetMachineType(const char* filePath) {
    FILE *elfFile = fopen(filePath, "rb");
    if (!elfFile) {
        return MachineType::ELF_EM_NONE;
    }

    Elf32_Ehdr elfHeader;
    fread(&elfHeader, sizeof(Elf32_Ehdr), 1, elfFile);
    fclose(elfFile);

    return static_cast<MachineType>(elfHeader.e_machine);
}

uintptr_t ELFParser::GetSymbolOffset32(uintptr_t entryAddr, Elf32_Ehdr* entryElf, const char* symbolName) {
    uintptr_t result = INVALID_SYMBOL_OFF;
    Elf32_Shdr* sections = (Elf32_Shdr*)((Elf32_Off)entryAddr + entryElf->e_shoff);
    Elf32_Shdr* symtab = nullptr;

    for (int i = 0; i < entryElf->e_shnum; i++) {
        if (sections[i].sh_type == SHT_SYMTAB || sections[i].sh_type == SHT_DYNSYM) {
            symtab = sections + i;
            break;
        }
    }

    if (!symtab) {
        return result;
    }

    const char* strSecAddr = (const char*)(entryAddr + sections[symtab->sh_link].sh_offset);
    Elf32_Sym* symSec = (Elf32_Sym*)(entryAddr + symtab->sh_offset);
    int nSymbols = symtab->sh_size / sizeof(Elf32_Sym);

    for (int i = 0; i < nSymbols; i++) {
        if (!(ELF32_ST_BIND(symSec[i].st_info) & (STT_FUNC | STB_GLOBAL))) {
            continue;
        }

        const char* currSymbolName = strSecAddr + symSec[i].st_name;
        if (strcmp(currSymbolName, symbolName) == 0) {
            result = symSec[i].st_value;
            break;
        }
    }

    return result;
}

uintptr_t ELFParser::GetSymbolOffset64(uintptr_t entryAddr, Elf64_Ehdr* entryElf, const char* symbolName) {
    uintptr_t result = INVALID_SYMBOL_OFF;
    Elf64_Shdr* sections = (Elf64_Shdr*)((Elf64_Off)entryAddr + entryElf->e_shoff);
    Elf64_Shdr* symtab = nullptr;

    for (int i = 0; i < entryElf->e_shnum; i++) {
        if (sections[i].sh_type == SHT_SYMTAB || sections[i].sh_type == SHT_DYNSYM) {
            symtab = sections + i;
            break;
        }
    }

    if (!symtab) {
        return result;
    }

    const char* strSecAddr = (const char*)(entryAddr + sections[symtab->sh_link].sh_offset);
    Elf64_Sym* symSec = (Elf64_Sym*)(entryAddr + symtab->sh_offset);
    int nSymbols = symtab->sh_size / sizeof(Elf64_Sym);

    for (int i = 0; i < nSymbols; i++) {
        if (!(ELF64_ST_BIND(symSec[i].st_info) & (STT_FUNC | STB_GLOBAL))) {
            continue;
        }

        const char* currSymbolName = strSecAddr + symSec[i].st_name;
        if (strcmp(currSymbolName, symbolName) == 0) {
            result = symSec[i].st_value;
            break;
        }
    }

    return result;
}

uintptr_t ELFParser::GetSymbolOffset(const char* elfPath, const char* symbolName) {
    uintptr_t result = INVALID_SYMBOL_OFF;

    int fd = open(elfPath, O_RDONLY);
    if (fd < 0) {
        return result;
    }

    union {
        void* entryRaw;
        uintptr_t entryAddr;
    };

    struct stat elfStat;
    if (fstat(fd, &elfStat) < 0) {
        close(fd);
        return result;
    }

    entryRaw = mmap(NULL, elfStat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (entryRaw == MAP_FAILED) {
        close(fd);
        return result;
    }

    auto machineType = GetMachineType(elfPath);

    if (machineType == MachineType::ELF_EM_X86_64 || machineType == MachineType::ELF_EM_AARCH64) {
        auto* entryElf64 = static_cast<Elf64_Ehdr*>(entryRaw);
        result = GetSymbolOffset64(entryAddr, entryElf64, symbolName);
    } else {
        auto* entryElf32 = static_cast<Elf32_Ehdr*>(entryRaw);
        result = GetSymbolOffset32(entryAddr, entryElf32, symbolName);
    }

    munmap(entryRaw, elfStat.st_size);
    close(fd);

    return result;
}