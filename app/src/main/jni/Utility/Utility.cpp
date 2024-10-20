//
// Created by reveny on 5/30/24.
//
#include <Include/Utility.hpp>

#include <sys/system_properties.h>

int Utility::GetSDKVersion() {
    char sdk_ver[32];
    __system_property_get("ro.build.version.sdk", sdk_ver);
    return atoi(sdk_ver);
}

std::string Utility::GetSystemProperty(const char *property) {
    char prop[PROP_VALUE_MAX];
    __system_property_get(property, prop);
    return {prop};
}

std::string Utility::GetProcessName() {
    FILE *file = fopen("/proc/self/cmdline", "r");
    if (file == nullptr) {
        return "";
    }

    char line[256];
    while (fgets(line, 256, file)) {
        fclose(file);
        return {line};
    }

    fclose(file);
    return "";
}

ELFParser::MachineType Utility::GetMachineType() {
    #if defined(__arm__)
        return ELFParser::MachineType::ELF_EM_ARM;
    #elif defined(__aarch64__)
        return ELFParser::MachineType::ELF_EM_AARCH64;
    #elif defined(__i386__)
        return ELFParser::MachineType::ELF_EM_386;
    #elif defined(__x86_64__)
        return ELFParser::MachineType::ELF_EM_X86_64;
    #else
        return ELFParser::MachineType::ELF_EM_NONE;
    #endif
}