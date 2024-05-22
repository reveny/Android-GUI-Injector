
package io.github.reveny.injector.util;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.net.Uri;
import android.widget.Toast;

import androidx.browser.customtabs.CustomTabColorSchemeParams;
import androidx.browser.customtabs.CustomTabsIntent;

import rikka.core.util.ResourceUtils;

public final class NavUtil {

    public static void startURL(Activity activity, Uri uri) {
        CustomTabsIntent.Builder customTabsIntent = new CustomTabsIntent.Builder();
        customTabsIntent.setShowTitle(true);
        CustomTabColorSchemeParams params = new CustomTabColorSchemeParams.Builder()
                .setToolbarColor(ResourceUtils.resolveColor(activity.getTheme(), android.R.attr.colorBackground))
                .setNavigationBarColor(ResourceUtils.resolveColor(activity.getTheme(), android.R.attr.navigationBarColor))
                .setNavigationBarDividerColor(0)
                .build();
        customTabsIntent.setDefaultColorSchemeParams(params);
        boolean night = ResourceUtils.isNightMode(activity.getResources().getConfiguration());
        customTabsIntent.setColorScheme(night ? CustomTabsIntent.COLOR_SCHEME_DARK : CustomTabsIntent.COLOR_SCHEME_LIGHT);
        try {
            customTabsIntent.build().launchUrl(activity, uri);
        } catch (ActivityNotFoundException ignored) {
            Toast.makeText(activity, uri.toString(), Toast.LENGTH_SHORT).show();
        }
    }

    public static void startURL(Activity activity, String url) {
        startURL(activity, Uri.parse(url));
    }
}
