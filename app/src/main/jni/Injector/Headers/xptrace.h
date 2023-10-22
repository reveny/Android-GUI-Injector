#pragma once

#include <sys/ptrace.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <cstdlib>

#include "../../Include/Logger.h"

inline long xptrace(long request, int pid, void *addr) {
    errno = 0; // Reset errno before the call
    long result = ptrace(request, pid, addr, nullptr);
    if (result == -1 && errno) {
        LOGE("[-] xptrace error %s in pid %d from request %ld", strerror(errno), pid, request);
        return -1;
    }
    return result;
}

inline long xptrace(long request, int pid) {
    errno = 0; // Reset errno before the call
    long result = ptrace(request, pid, nullptr, nullptr);
    if (result == -1 && errno) {
        LOGE("[-] xptrace error %s in pid %d from request %ld", strerror(errno), pid, request);
        return -1;
    }
    return result;
}

template<typename T>
long xptrace(long request, pid_t pid, void* addr, T&& data) {
    errno = 0; // Reset errno before the call
    long result = ptrace(request, pid, addr, std::forward<T>(data));
    if (result == -1 && errno) {
        LOGE("[-] xptrace error %s in pid %d from request %ld", strerror(errno), pid, request);
        return -1;
    }
    return result;
}