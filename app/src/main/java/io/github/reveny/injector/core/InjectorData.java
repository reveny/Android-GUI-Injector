package io.github.reveny.injector.core;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Messenger;

public class InjectorData {
    public static InjectorData instance;

    private String packageName;
    private String launcherActivity;
    private String libraryPath;

    private boolean shouldAutoLaunch;
    private boolean shouldKillBeforeLaunch;

    private boolean injectZygote;
    private boolean remapLibrary;

    private boolean useProxy;
    private boolean randomizeProxyName;
    private boolean copyToCache;
    private boolean hideLibrary;

    // Until I figure out how to steal the code from xdl, this requires usage of the proxy
    private boolean bypassNamespaceRestrictions;
    
    public InjectorData() {
        instance = this;
        this.shouldAutoLaunch = false;
        this.shouldKillBeforeLaunch = false;
        this.injectZygote = false;
        this.remapLibrary = false;
        this.useProxy = false;
        this.randomizeProxyName = false;
        this.copyToCache = false;
        this.bypassNamespaceRestrictions = false;
    }
    
    // Implement get and set methods
    public String getPackageName() {
        return packageName;
    }

    public void setPackageName(String packageName) {
        this.packageName = packageName;
    }

    public String getLauncherActivity() {
        return launcherActivity;
    }

    public void setLauncherActivity(String launcherActivity) {
        this.launcherActivity = launcherActivity;
    }

    public String getLibraryPath() {
        return libraryPath;
    }

    public void setLibraryPath(String libraryPath) {
        this.libraryPath = libraryPath;
    }

    public boolean isShouldAutoLaunch() {
        return shouldAutoLaunch;
    }

    public void setShouldAutoLaunch(boolean shouldAutoLaunch) {
        this.shouldAutoLaunch = shouldAutoLaunch;
    }

    public boolean isShouldKillBeforeLaunch() {
        return shouldKillBeforeLaunch;
    }

    public void setShouldKillBeforeLaunch(boolean shouldKillBeforeLaunch) {
        this.shouldKillBeforeLaunch = shouldKillBeforeLaunch;
    }

    public boolean isInjectZygote() {
        return injectZygote;
    }

    public void setInjectZygote(boolean injectZygote) {
        this.injectZygote = injectZygote;
    }

    public boolean isRemapLibrary() {
        return remapLibrary;
    }

    public void setRemapLibrary(boolean remapLibrary) {
        this.remapLibrary = remapLibrary;
    }

    public boolean isUseProxy() {
        return useProxy;
    }

    public void setUseProxy(boolean useProxy) {
        this.useProxy = useProxy;
    }

    public boolean isRandomizeProxyName() {
        return randomizeProxyName;
    }

    public void setRandomizeProxyName(boolean randomizeProxyName) {
        this.randomizeProxyName = randomizeProxyName;
    }

    public boolean isCopyToCache() {
        return copyToCache;
    }

    public void setCopyToCache(boolean copyToCache) {
        this.copyToCache = copyToCache;
    }

    public boolean isHideLibrary() {
        return hideLibrary;
    }

    public void setHideLibrary(boolean hideLibrary) {
        this.hideLibrary = hideLibrary;
    }

    public boolean isBypassNamespaceRestrictions() {
        return bypassNamespaceRestrictions;
    }

    public void setBypassNamespaceRestrictions(boolean bypassNamespaceRestrictions) {
        this.bypassNamespaceRestrictions = bypassNamespaceRestrictions;
    }

    public Message toMessage(int what) {
        Message message = Message.obtain((Handler) null, what);
        Bundle data = message.getData();

        data.putString("packageName", packageName);
        data.putString("launcherActivity", launcherActivity);
        data.putString("libraryPath", libraryPath);

        data.putBoolean("shouldAutoLaunch", shouldAutoLaunch);
        data.putBoolean("shouldKillBeforeLaunch", shouldKillBeforeLaunch);

        data.putBoolean("injectZygote", injectZygote);
        data.putBoolean("remapLibrary", remapLibrary);
        data.putBoolean("useProxy", useProxy);
        data.putBoolean("randomizeProxyName", randomizeProxyName);
        data.putBoolean("copyToCache", copyToCache);
        data.putBoolean("hideLibrary", hideLibrary);
        data.putBoolean("bypassLinkerRestrictions", bypassNamespaceRestrictions);

        message.setData(data);
        return message;
    }

    public InjectorData fromMessage(Message message) {
        Bundle data = message.getData();

        this.packageName = data.getString("packageName");
        this.launcherActivity = data.getString("launcherActivity");
        this.libraryPath = data.getString("libraryPath");

        this.shouldAutoLaunch = data.getBoolean("shouldAutoLaunch");
        this.shouldKillBeforeLaunch = data.getBoolean("shouldKillBeforeLaunch");

        this.injectZygote = data.getBoolean("injectZygote");
        this.remapLibrary = data.getBoolean("remapLibrary");
        this.useProxy = data.getBoolean("useProxy");
        this.randomizeProxyName = data.getBoolean("randomizeProxyName");
        this.copyToCache = data.getBoolean("copyToCache");
        this.hideLibrary = data.getBoolean("hideLibrary");
        this.bypassNamespaceRestrictions = data.getBoolean("bypassLinkerRestrictions");

        return this;
    }

    @Override
    public String toString() {
        return "InjectorData{" +
        "packageName='" + packageName + '\'' +
        ", launcherActivity='" + launcherActivity + '\'' +
        ", libraryPath='" + libraryPath + '\'' +
        ", shouldAutoLaunch=" + shouldAutoLaunch +
        ", shouldKillBeforeLaunch=" + shouldKillBeforeLaunch +
        ", injectZygote=" + injectZygote +
        ", remapLibrary=" + remapLibrary +
        ", useProxy=" + useProxy +
        ", randomizeProxyName=" + randomizeProxyName +
        ", copyToCache=" + copyToCache +
        ", hideLibrary=" + hideLibrary +
        ", bypassNamespaceRestrictions=" + bypassNamespaceRestrictions +
        '}';
    }
}
