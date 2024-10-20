//
// Created by reveny on 5/17/24.
//
#include <sys/stat.h>
#include <random>
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>

#include <Injector.hpp>
#include <InjectorData.hpp>
#include <Utility.hpp>
#include <RevMemory.hpp>

#include <Logger.hpp>
#include <pwd.h>
#include <grp.h>

extern "C" jint Injector::Inject(JNIEnv* env, jclass clazz, jobject data) {
    (void)clazz;
    LOGI("[+] Inject called");

    std::shared_ptr<InjectorData> injectorData = std::make_shared<InjectorData>(env, data);

    // Handle SELinux
    // NOTE: Causes crashes on emulators, most emulators don't need selinux
    RevMemory::SetSELinux(0);

    // Since the native library is likely at a place like /sdcard/ we need to make sure
    // to copy it to a directory where it can be executed from.
    // We can just copy it to a random directory in /data/local/tmp
    std::filesystem::path currentPath = injectorData->getLibraryPath();
    std::string tmpDir = CreateRandomTempDirectory(injectorData->getPackageName());
    if (tmpDir.empty()) {
        LOGE("[-] Failed to create temporary directory, aborting injection.");
        return -1;
    }

    // On Android 11+ we can't write to /data/data/package/cache, so we have to copy it to /data/local/tmp
    // I have removed this feature for now to not cause any confusion.
    std::string tmpLibraryPath = tmpDir + "/" + currentPath.filename().string();

    try {
        CopyFile(currentPath, tmpLibraryPath);
    } catch (const std::exception& e) {
        LOGE("[-] Failed to copy library: %s", e.what());
        return -1;
    }
    injectorData->setLibraryPath(tmpLibraryPath);

    // Auto launch the app if enabled
    if (injectorData->getShouldAutoLaunch()) {
        LOGI("[+] Auto launching app...");
        RevMemory::LaunchApp(injectorData->getLauncherActivity());
    }

    // Wait until process is ready
    pid_t processID = RevMemory::WaitForProcess(injectorData->getPackageName());

    // Adjust the library path if proxy is enabled
    if (injectorData->getUseProxy()) {
        // Get library directory
        std::string libraryPath = RevMemory::GetNativeLibraryDirectory() + "libRevenyProxy.so";

        // If proxy renaming is enabled, we need to copy it to a different
        // directory and rename it. We copy it to /data/local/tmp/random/name.so
        if (injectorData->getRandomizeProxyName()) {
            std::string tmpLibraryPath = tmpDir + "/lib" + GenerateRandomString() + ".so";

            try {
                CopyFile(libraryPath, tmpLibraryPath);
            } catch (const std::exception& e) {
                LOGE("[-] Failed to copy proxy library: %s", e.what());
                return -1;
            }
            libraryPath = tmpLibraryPath;
        }

        // Inject proxy, we pass the injector data here because
        // we need to pass everything to the proxy that it needs to do.
        pid_t pid = RevMemory::FindProcessID(injectorData->getPackageName());
        int result = RevMemory::InjectProxy(pid, libraryPath, injectorData);
        LOGI("[+] Finished Proxy Injection with result %d", result);

        // Handle SELinux
        RevMemory::SetSELinux(1);

        return result;
    }

    // Check if we are on an emulator and then check the target library.
    // If the target library fits is also a x86/x86_64 library, we can
    // just load it normally, if not we have to do native bridge injection.
    // unless proxy is enabled.
    ELFParser::MachineType machine = Utility::GetMachineType();
    ELFParser::MachineType library = ELFParser::GetMachineType(injectorData->getLibraryPath().c_str());
    if ((machine == ELFParser::MachineType::ELF_EM_386 || machine == ELFParser::MachineType::ELF_EM_X86_64)
    && (library == ELFParser::MachineType::ELF_EM_AARCH64 || library == ELFParser::MachineType::ELF_EM_ARM)) {
        LOGI("[+] Detected arm or aarch64 library on Emulator, starting emulator injection...");

        int result = RevMemory::EmulatorInject(processID, injectorData->getLibraryPath(), injectorData->getRemapLibrary());
        LOGI("[+] Finished Emulator Injection with result %d", result);

        return result;
    }

    // Inject Normally
    LOGI("[+] Starting Injection...");
    int result = RevMemory::Inject(processID, injectorData->getLibraryPath(), injectorData->getRemapLibrary());
    LOGI("[+] Finished Injection with result %d", result);

    // Handle SELinux
    RevMemory::SetSELinux(1);

    return result;
}

extern "C" jobjectArray Injector::GetNativeLogs(JNIEnv *env, jclass clazz) {
    (void)clazz;

    LOGI("[+] GetNativeLogs called: Logging %d messages", log_messages.size());
    jobjectArray ret{};

    ret = (jobjectArray)env->NewObjectArray((int)log_messages.size(), env->FindClass("java/lang/String"),env->NewStringUTF(""));
    for (size_t i = 0; i < log_messages.size(); ++i) {
        env->SetObjectArrayElement(ret, static_cast<jlong>(i), env->NewStringUTF(log_messages[i].c_str()));
    }

    return ret;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void)reserved;

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    LOGI("[+] JNI_OnLoad called");

    // Register Natives
    JNINativeMethod methods[] = {
        {"Inject", "(Lio/github/reveny/injector/core/InjectorData;)I", reinterpret_cast<void*>(Injector::Inject)},
        {"GetNativeLogs", "()[Ljava/lang/String;", reinterpret_cast<void*>(Injector::GetNativeLogs)}
    };

    jclass clazz = env->FindClass("io/github/reveny/injector/core/Native");
    int rc = env->RegisterNatives(clazz, methods, sizeof(methods)/sizeof(JNINativeMethod));
    if (rc != JNI_OK)
    {
        LOGE("[-] JNI_OnLoad: Error initializing");
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

void Injector::CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination) {
    try {
        if (!std::filesystem::exists(source)) {
            throw std::runtime_error("Source file does not exist.");
        }

        if (!std::filesystem::exists(destination.parent_path())) {
            throw std::runtime_error(("Destination directory " + destination.parent_path().string() + " does not exist."));
        }

        std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing);
        LOGI("[+] File %s copied successfully to %s.", source.c_str(), destination.c_str());

        // Set permissions
        if (chmod(destination.string().c_str(), 0777) == -1) {
            LOGE("[-] Failed to set permissions on %s ([%d] %s)", destination.c_str(), errno, strerror(errno));
            return;
        }

        struct stat st;
        if (stat("/sdcard/Documents", &st) == -1) {
            LOGE("[-] Failed to stat /sdcard/Documents ([%d] %s)", errno, strerror(errno));
            return;
        }

        // Set the ownership to 'shell' user and group, I think that's not necessary
        // but I encountered some issues on emulators.
        struct passwd *pwd = getpwnam("shell");
        if (pwd == NULL) {
            LOGE("[-] Failed to get 'shell' user information");
        }
        uid_t uid = pwd->pw_uid;

        struct group *grp = getgrnam("shell");
        if (grp == NULL) {
            LOGE("[-] Failed to get 'shell' group information");
        }
        gid_t gid = grp->gr_gid;

        if (chown(destination.string().c_str(), uid, gid) == -1) {
            LOGE("[-] Failed to change ownership of directory %s ([%d] %s)", destination.string().c_str(), errno, strerror(errno));
        }

        // For some reason this causes an error?
        // system(("chmod +x " + destination.string()).c_str());
    } catch (const std::filesystem::filesystem_error& e) {
        LOGE("[-] Filesystem error: %s", e.what());
    } catch (const std::runtime_error& e) {
        LOGE("[-] Runtime error: %s", e.what());
    } catch (const std::exception& e) {
        LOGE("[-] General exception: %s", e.what());
    } catch (...) {
        LOGE("[-] Unknown error occurred.");
    }
}

std::string Injector::GenerateRandomString() {
    const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, (int)characters.size() - 1);

    std::string randomString;
    randomString.reserve(10);

    for (size_t i = 0; i < 10; ++i) {
        randomString += characters[static_cast<unsigned int>(distribution(generator))];
    }

    return randomString;
}

// Not really random but it used to be before changing some of the logic
std::string Injector::CreateRandomTempDirectory(std::string packageName) {
    // Create random directory and return the path so it can be used later.
    std::string tmpDir = "/data/local/tmp/inject/" + packageName; // + GenerateRandomString(10);

    // Stat download directory to know target UID and GID. The issue is that we create this
    // tmp directory as root which means that no app can access it and open the library
    // that's why we just copy the permissions from /sdcard/Documents
    struct stat st;
    if (stat("/sdcard/Documents", &st) == -1) {
        LOGE("[-] Failed to stat /sdcard/Documents ([%d] %s)", errno, strerror(errno));
        return "";
    }

    uid_t uid = st.st_uid;
    gid_t gid = st.st_gid;

    // TODO: Clean this up
    // Create inject directory first if it doesn't exist
    if (mkdir("/data/local/tmp/inject", 0777) == -1 && errno != EEXIST) {
        LOGE("[-] Failed to create directory /data/local/tmp/inject ([%d] %s)", errno, strerror(errno));
        return "";
    }

    if (chown("/data/local/tmp/inject", uid, gid) == -1) {
        LOGE("[-] Failed to change ownership of directory /data/local/tmp/inject ([%d] %s)", errno, strerror(errno));
    }

    if (chmod("/data/local/tmp/inject", 0777) == -1) {
        LOGE("[-] Failed to set permissions on directory /data/local/tmp/inject ([%d] %s)", errno, strerror(errno));
    }

    if (mkdir(tmpDir.c_str(), 0777) == -1 && errno != EEXIST) {
        LOGE("[-] Failed to create directory %s (%s)", tmpDir.c_str(), strerror(errno));
        return "";
    }

    if (chown(tmpDir.c_str(), uid, gid) == -1) {
        LOGE("[-] Failed to change ownership of directory %s ([%d] %s)", tmpDir.c_str(), errno, strerror(errno));
    }

    if (chmod(tmpDir.c_str(), 0777) == -1) {
        LOGE("[-] Failed to set permissions on directory %s ([%d] %s)", tmpDir.c_str(), errno, strerror(errno));
    }

    return tmpDir;
}