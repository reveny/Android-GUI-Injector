package com.reveny.injector.v2;

public class Native {
    public static native int Inject(String pkg, String libPath, String launcherAct, boolean autoLaunch, boolean killBeforeLaunch, boolean remapLibrary);
    public static native String[] GetNativeLogs();

    static {
        System.loadLibrary("RevenyInjector");
    }
}
