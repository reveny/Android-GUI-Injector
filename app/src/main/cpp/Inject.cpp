#include "Injector/Injector.h"
#include <stdio.h>
#include "Logger.h"
#include "jni.h"
#include <vector>

extern "C" {
    JNIEXPORT jint JNICALL
    Java_com_reveny_injector_Native_Inject(JNIEnv *env, jclass clazz, jstring package_name, jstring library_path, jstring launcherAct, jboolean auto_launch) {
        pkgName = env->GetStringUTFChars(package_name, nullptr);
        libraryPath = env->GetStringUTFChars(library_path, nullptr);
        shouldAutoLaunch = auto_launch;
        appLaunchActivity = env->GetStringUTFChars(launcherAct, nullptr);;

        return initInject();
    }
}