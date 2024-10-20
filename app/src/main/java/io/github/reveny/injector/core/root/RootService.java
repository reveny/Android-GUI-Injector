package io.github.reveny.injector.core.root;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;

import androidx.annotation.NonNull;

import com.topjohnwu.superuser.Shell;

import java.util.ArrayList;
import java.util.List;

import io.github.reveny.injector.core.InjectorData;
import io.github.reveny.injector.core.LogManager;
import io.github.reveny.injector.core.Native;

public class RootService extends com.topjohnwu.superuser.ipc.RootService implements Handler.Callback {
    @Override
    public IBinder onBind(@NonNull Intent intent) {
        LogManager.AddLog("RootService: onBind");

        Handler handler = new Handler(Looper.getMainLooper(), this);
        return new Messenger(handler).getBinder();
    }

    @Override
    public boolean handleMessage(@NonNull Message msg) {
        LogManager.AddLog("RootService: handleMessage");

        if (msg.what != 1) {
            return false;
        }

        InjectorData data = new InjectorData().fromMessage(msg);
        int result = Native.Inject(data);
        String[] logs = Native.GetNativeLogs();

        Message message = Message.obtain();
        Bundle bundle = new Bundle();
        bundle.putInt("result", result);
        bundle.putStringArray("logs", logs);
        message.setData(bundle);

        try {
            msg.replyTo.send(message);
        } catch (Exception e) {
            LogManager.AddLog("Failed to send message: " + e.getMessage());
        }

        return false;
    }
}
