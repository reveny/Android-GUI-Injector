package io.github.reveny.injector.core;

import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class LogManager {
    public static final String TAG = "RevenyInjector";
    public static List<String> logs = new ArrayList<String>();

    public static void AddLog(String log) {
        Log.i(TAG, log);
        logs.add(log);
    }

    public static List<String> GetLogs() {
        return logs;
    }
}
