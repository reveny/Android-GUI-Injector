//
// Created by reveny on 20/10/2023.
//

#include "../Include/Logger.h"
#include "Headers/RevMemory.h"

#include <cstdio>
#include <jni.h>
#include <vector>
#include <unistd.h>

extern "C" {
    JNIEXPORT jint JNICALL
    Java_com_reveny_injector_v2_Native_Inject(JNIEnv *env, jclass clazz, jstring pkg, jstring lib_path, jstring launcher_act, jboolean auto_launch, jboolean kill_before_launch, jboolean remap_library) {
        const char *native_pkg = env->GetStringUTFChars(pkg, nullptr);
        const char *native_library = env->GetStringUTFChars(lib_path, nullptr);

        // Launch Game if option is used
        if (auto_launch) {
            RevMemory::launch_app(env->GetStringUTFChars(launcher_act, nullptr));
        }

        // Wait until the process is found
        pid_t pid = -1;
        while (pid <= 0) {
            pid = RevMemory::find_process_id(native_pkg);
            sleep(1);
        }
        LOGI("[+] Found Target process %s with pid %d", native_pkg, pid);

        // Handle SELinux
        // NOTE: Causes crashes on emulators, most emulators don't need selinux
        bool is_emulator = false;
        #if defined(__arm__) || defined(__aarch64__)
            is_emulator = false;
            RevMemory::set_selinux(0);
        #elif defined(__x86_64__) || defined(__i386__)
            is_emulator = true;
        #endif

        // Check if we are on an emulator and then check the target library.
        // If the target library fits is also a x86/x86_64 library, we can
        // just load it normally, if not we have to do native bridge injection.
        if (is_emulator) {
            // Check Library Architecture
            ELFParser::MachineType type = ELFParser::getMachineType(native_library);

            // Needs to be loaded through native bridge
            if (type == ELFParser::MachineType::ELF_EM_AARCH64 || type == ELFParser::MachineType::ELF_EM_ARM) {
                LOGI("[+] Detected arm or aarch64 library on Emulator, starting emulator injection...");
                int result = RevMemory::EmulatorInject(pid, native_library, remap_library);
                LOGI("[+] Finished Emulator Injection with result %d", result);

                // Return the injection result
                return result;
            }
        }

        // Inject Normally
        int result = RevMemory::Inject(pid, native_library, remap_library);
        LOGI("[+] Finished Injection with result %d", result);

        // Restore SELinux
        #if defined(__arm__) || defined(__aarch64__)
            RevMemory::set_selinux(1);
        #endif

        return result;
    }

    JNIEXPORT jobjectArray JNICALL
    Java_com_reveny_injector_v2_Native_GetNativeLogs(JNIEnv *env, jclass clazz) {
        std::vector<std::string> local_log = log_messages;
        jobjectArray ret{};

        ret = (jobjectArray)env->NewObjectArray((int)local_log.size(), env->FindClass("java/lang/String"),env->NewStringUTF(""));
        for (int i = 0; i < local_log.size(); ++i) {
            env->SetObjectArrayElement(ret, i, env->NewStringUTF(local_log[i].c_str()));
        }

        return ret;
    }
}