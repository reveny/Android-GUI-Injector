package com.reveny.injector.v2;

public class Native {
    public static native int Inject(String pkg, String libPath, String launcherAct, boolean autoLaunch, boolean killBeforeLaunch, boolean remapLibrary);

    static {
        System.loadLibrary("RevenyInjector");
    }
}
