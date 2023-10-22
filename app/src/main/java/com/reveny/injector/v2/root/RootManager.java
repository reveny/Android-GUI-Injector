package com.reveny.injector.v2.root;

import android.util.Log;

import com.reveny.injector.v2.LogManager;
import com.topjohnwu.superuser.Shell;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

public class RootManager {
    public static RootManager instance = new RootManager();

    static {
        // Set settings before the main shell can be created
        Shell.enableVerboseLogging = true;
        Shell.setDefaultBuilder(Shell.Builder.create()
            .setFlags(Shell.FLAG_REDIRECT_STDERR)
            .setTimeout(10)
        );
    }

    public boolean hasRootAccess = false;

    public void RequestRoot() {
        LogManager.AddLog("Requesting Root Permission...");

        Shell.getShell(shell -> {
            LogManager.AddLog("Root Permission Granted");
            hasRootAccess = true;
        });
    }

    public boolean isAppRunning(String packageName) {
        List<String> res = new ArrayList<>();
        Shell.cmd("pidof " + packageName).to(res).exec();

        if (res.size() == 0) {
            return false;
        } else {
            return true;
        }
    }

    public String getPid(String packageName) {
        List<String> res = new ArrayList<>();
        Shell.cmd("pidof " + packageName).to(res).exec();

        if (res.size() == 0) {
            return "-1";
        } else {
            return res.get(0);
        }
    }
}
