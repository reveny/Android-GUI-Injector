package io.github.reveny.injector.core;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.webkit.MimeTypeMap;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;

public class Utility {
    public static List<String> getInstalledApps(Context context) {
        List<ApplicationInfo> packages = context.getPackageManager().getInstalledApplications(PackageManager.GET_META_DATA);
        List<String> ret = new ArrayList<>();

        for (ApplicationInfo appInfo : packages) {
            // Check if the application is not a system app and not the current app
            if ((appInfo.flags & ApplicationInfo.FLAG_SYSTEM) == 0 && !appInfo.packageName.equals(context.getPackageName())) {
                ret.add(appInfo.packageName);
            }
        }

        return ret;
    }

    public static String getFileExtension(Uri uri) {
        String path = uri.getPath();
        if (path != null) {
            int lastDot = path.lastIndexOf('.');
            if (lastDot != -1) {
                return path.substring(lastDot + 1);
            }
        }
        return null;
    }

    public static boolean isEmulator() {
        BufferedReader reader;

        try {
            reader = new BufferedReader(new FileReader("/proc/cpuinfo"));
            String line = reader.readLine();

            while (line != null) {
                if (line.contains("hypervisor") || line.contains("amd") || line.contains("intel")) {
                    return true;
                }

                line = reader.readLine();
            }

            reader.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return false;
    }

    public static boolean isRooted() {
        File f = new File("/system/bin/su");
        return f.exists() && !f.isDirectory();
    }

    public static String getLaunchActivity(Context context, String packageName) {
        String activityName = "";
        final PackageManager pm = context.getPackageManager();
        Intent intent = pm.getLaunchIntentForPackage(packageName);
        if (intent != null) {
            List<ResolveInfo> activityList = pm.queryIntentActivities(intent,0);
            activityName = activityList.get(0).activityInfo.name;
            return packageName + "/" + activityName;
        }
        return "";
    }
}
