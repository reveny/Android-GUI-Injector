package io.github.reveny.injector.core.root;

import com.topjohnwu.superuser.Shell;

import java.util.ArrayList;
import java.util.List;

import io.github.reveny.injector.core.LogManager;

public class RootManager {
    public static RootManager instance;

    public RootManager() {
        instance = this;

        // Set settings before the main shell can be created
        Shell.enableVerboseLogging = true;
        Shell.setDefaultBuilder(Shell.Builder.create()
            .setFlags(Shell.FLAG_REDIRECT_STDERR)
            .setTimeout(10)
        );
    }

    public boolean hasRootAccess = false;

    public void requestRoot() {
        LogManager.AddLog("Requesting Root Permission...");

        Shell.getShell(shell -> {
            LogManager.AddLog("Root Permission Granted: " + Shell.isAppGrantedRoot());
            hasRootAccess = Boolean.TRUE.equals(Shell.isAppGrantedRoot());
        });
    }

    public boolean isAppRunning(String packageName) {
        List<String> res = new ArrayList<>();
        Shell.cmd("pidof " + packageName).to(res).exec();

        return !res.isEmpty();
    }

    public String getPid(String packageName) {
        List<String> res = new ArrayList<>();
        Shell.cmd("pidof " + packageName).to(res).exec();

        return res.isEmpty() ? "-1" : res.get(0);
    }
}
