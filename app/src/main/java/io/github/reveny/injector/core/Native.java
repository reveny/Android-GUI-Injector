package io.github.reveny.injector.core;

public class Native {
    static {
        System.loadLibrary("RevenyInjector");
    }

    public static native int Inject(InjectorData data);
    public static native String[] GetNativeLogs();
}
