//
// Created by reveny on 21/10/2023.
//
#pragma once

#include <elf.h>
#include <stdint.h>

namespace ELFParser {
    const size_t INVALID_SYMBOL_OFF = static_cast<size_t>(-1);

    enum class MachineType : uint16_t {
        ELF_EM_NONE = EM_NONE,       // No machine
        ELF_EM_386 = EM_386,         // Intel 80386
        ELF_EM_ARM = EM_ARM,         // ARM
        ELF_EM_X86_64 = EM_X86_64,   // x86_64
        ELF_EM_AARCH64 = EM_AARCH64  // ARM64
    };

    MachineType GetMachineType(const char* filePath);

    /**
     * @brief Get the offset of a symbol in a 32-bit ELF file.
     *
     * @param entryAddr The starting address of the mapped ELF file.
     * @param entryElf Pointer to the ELF header.
     * @param symbolName The name of the symbol to find.
     * @return size_t The offset of the symbol or INVALID_SYMBOL_OFF if not found.
     */
    uintptr_t GetSymbolOffset32(uintptr_t entryAddr, Elf32_Ehdr* entryElf, const char* symbolName);

    /**
     * @brief Get the offset of a symbol in a 64-bit ELF file.
     *
     * @param entryAddr The starting address of the mapped ELF file.
     * @param entryElf Pointer to the ELF header.
     * @param symbolName The name of the symbol to find.
     * @return size_t The offset of the symbol or INVALID_SYMBOL_OFF if not found.
     */
    uintptr_t GetSymbolOffset64(uintptr_t entryAddr, Elf64_Ehdr* entryElf, const char* symbolName);

    /**
     * @brief Get the offset of a symbol in an ELF file.
     *
     * @param elfPath Path to the ELF file.
     * @param symbolName The name of the symbol to find.
     * @return size_t The offset of the symbol or INVALID_SYMBOL_OFF if not found.
     */
    uintptr_t GetSymbolOffset(const char* elfPath, const char* symbolName);
}
