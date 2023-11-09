package com.reveny.injector.v2;

import android.util.Log;

import com.reveny.injector.v2.ui.LogsFragment;

import java.util.ArrayList;
import java.util.List;

public class LogManager {
    public static String TAG = "RevenyInjector";
    private static List<String> logs = new ArrayList<String>();

    public static void AddLog(String log) {
        logs.add(log);
    }

    public static List<String> GetLogs() {
        return logs;
    }
}
