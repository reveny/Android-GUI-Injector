package io.github.reveny.injector.core.root;

import android.app.Activity;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.widget.Toast;

import androidx.annotation.NonNull;

import java.util.Arrays;

import io.github.reveny.injector.core.LogManager;

public class RootHandler implements Handler.Callback {
    public static RootHandler instance;
    public Activity activity;

    // Root Connection
    public MessageConnection messageConnection;
    public Messenger remoteMessenger;
    public final Messenger replyMessenger = new Messenger(new Handler(Looper.getMainLooper(), this));

    public RootHandler() {
        instance = this;
    }

    public void Inject(Activity activity) {
        this.activity = activity;
        if (messageConnection == null) {
            RootService.bind(new Intent(activity, RootService.class), new MessageConnection(this));
        } else {
            RootService.unbind(messageConnection);
        }
    }

    @Override
    public boolean handleMessage(@NonNull Message msg) {
        int result = msg.getData().getInt("result");

        if (result == -1) {
            LogManager.AddLog("Injection failed: " + result);
            Toast.makeText(activity, "Injection failed", Toast.LENGTH_SHORT).show();
        } else {
            LogManager.AddLog("Injection success: " + result);
            Toast.makeText(activity, "Injection success", Toast.LENGTH_SHORT).show();
        }

        // Add Log from native
        String[] logs = msg.getData().getStringArray("logs");
        if (logs == null) {
            LogManager.AddLog("Failed to get native Log");
            return false;
        }

        LogManager.logs.addAll(Arrays.asList(logs));

        return false;
    }
}
