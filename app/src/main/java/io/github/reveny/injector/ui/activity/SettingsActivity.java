package io.github.reveny.injector.ui.activity;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;

import androidx.annotation.NonNull;

import io.github.reveny.injector.App;
import io.github.reveny.injector.R;
import io.github.reveny.injector.ui.fragment.SettingsFragment;

public class SettingsActivity extends BaseActivity {

    private static final String KEY_PREFIX = SettingsActivity.class.getName() + '.';
    private static final String EXTRA_SAVED_INSTANCE_STATE = KEY_PREFIX + "SAVED_INSTANCE_STATE";

    private boolean restarting;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.fragment_settings);
        if (savedInstanceState == null) {
            getSupportFragmentManager()
                    .beginTransaction()
                    .replace(R.id.fragment_container, new SettingsFragment())
                    .commit();
        }
    }

    @NonNull
    public static Intent newIntent(@NonNull Context context) {
        return new Intent(context, SettingsActivity.class);
    }

    @NonNull
    private static Intent newIntent(@NonNull Bundle savedInstanceState, @NonNull Context context) {
        return newIntent(context)
                .putExtra(EXTRA_SAVED_INSTANCE_STATE, savedInstanceState);
    }
    public void restart() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S || App.isParasitic) {
            recreate();
        } else {
            try {
                Bundle savedInstanceState = new Bundle();
                onSaveInstanceState(savedInstanceState);
                finish();
                startActivity(newIntent(savedInstanceState, this));
                overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
                restarting = true;
            } catch (Throwable e) {
                recreate();
            }
        }
    }
    @Override
    public boolean dispatchKeyEvent(@NonNull KeyEvent event) {
        return restarting || super.dispatchKeyEvent(event);
    }

    @SuppressLint("RestrictedApi")
    @Override
    public boolean dispatchKeyShortcutEvent(@NonNull KeyEvent event) {
        return restarting || super.dispatchKeyShortcutEvent(event);
    }

    @Override
    public boolean dispatchTouchEvent(@NonNull MotionEvent event) {
        return restarting || super.dispatchTouchEvent(event);
    }

    @Override
    public boolean dispatchTrackballEvent(@NonNull MotionEvent event) {
        return restarting || super.dispatchTrackballEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(@NonNull MotionEvent event) {
        return restarting || super.dispatchGenericMotionEvent(event);
    }
}
