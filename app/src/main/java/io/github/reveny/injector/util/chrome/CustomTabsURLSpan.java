package io.github.reveny.injector.util.chrome;

import android.app.Activity;
import android.text.style.URLSpan;
import android.view.View;

import io.github.reveny.injector.util.NavUtil;

public class CustomTabsURLSpan extends URLSpan {

    private final Activity activity;

    public CustomTabsURLSpan(Activity activity, String url) {
        super(url);
        this.activity = activity;
    }

    @Override
    public void onClick(View widget) {
        String url = getURL();
        NavUtil.startURL(activity, url);
    }
}
