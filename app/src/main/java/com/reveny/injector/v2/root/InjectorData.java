package com.reveny.injector.v2.root;

public class InjectorData {
    public String packageName;
    public String launcherActivity;
    public String libraryPath;

    public boolean isAppRunning;
    public String processID;

    InjectionMethods injectionMethod;

    public boolean shouldAutoLaunch;
    public boolean shouldKillBeforeLaunch;

    public boolean remapLibrary;


    enum InjectionMethods {
        Ptrace,
        LDpreload,
        Xposed,
    }
}
