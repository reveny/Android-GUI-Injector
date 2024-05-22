package io.github.reveny.injector;

import android.app.Application;
import android.content.SharedPreferences;
import android.os.Process;
import android.text.TextUtils;

import androidx.appcompat.app.AppCompatDelegate;
import androidx.preference.PreferenceManager;

import java.util.Locale;

import io.github.reveny.injector.core.root.RootManager;
import io.github.reveny.injector.util.ThemeUtil;
import okhttp3.Cache;
import okhttp3.OkHttpClient;
import rikka.material.app.LocaleDelegate;

public class App extends Application {
    private static App instance = null;
    private SharedPreferences pref;

    public static App getInstance() {
        return instance;
    }

    public static SharedPreferences getPreferences() {
        return instance.pref;
    }

    public static final boolean isParasitic = !Process.isApplicationUid(Process.myUid());

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        pref = PreferenceManager.getDefaultSharedPreferences(this);
        AppCompatDelegate.setDefaultNightMode(ThemeUtil.getDarkTheme());
        LocaleDelegate.setDefaultLocale(getLocale());
        var res = getResources();
        var config = res.getConfiguration();
        config.setLocale(LocaleDelegate.getDefaultLocale());
        res.updateConfiguration(config, res.getDisplayMetrics());

        // Setup root
        RootManager manager = new RootManager();
        manager.requestRoot();
    }

    public static Locale getLocale(String tag) {
        if (TextUtils.isEmpty(tag) || "SYSTEM".equals(tag)) {
            return LocaleDelegate.getSystemLocale();
        }
        return Locale.forLanguageTag(tag);
    }

    public static Locale getLocale() {
        String tag = getPreferences().getString("language", null);
        return getLocale(tag);
    }
}
