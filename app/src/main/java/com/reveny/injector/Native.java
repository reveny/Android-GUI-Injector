package com.reveny.injector;

public class Native {
    public static native int Inject(String pkg, String libPath, String launcherAct, boolean autoLaunch);

    static {
        System.loadLibrary("RevenyInjector");
    }
}
