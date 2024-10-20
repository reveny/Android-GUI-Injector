//
// Created by reveny on 5/30/24.
//
#pragma once

#include <jni.h>
#include <string>

#include <Include/InjectorData.hpp>

namespace JNIProxy {
    auto GetCreatedJavaVMS();

    int JNILoad(JavaVM *vm, std::string libraryPath);
    int Inject(RemoteInjectorData *data);
}