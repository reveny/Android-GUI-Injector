package com.reveny.injector.v2;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;

public class Utility {
    public static void OpenURL(Activity activity, String url) {
        Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
        activity.startActivity(browserIntent);
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
        if (f.exists() && !f.isDirectory()) {
            return true;
        }
        return false;
    }
}
