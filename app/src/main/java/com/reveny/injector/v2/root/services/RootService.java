package com.reveny.injector.v2.root.services;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

import androidx.annotation.NonNull;

import com.reveny.injector.v2.Native;
import com.reveny.injector.v2.ui.InjectFragment;

public class RootService extends com.topjohnwu.superuser.ipc.RootService implements Handler.Callback {

    @Override
    public IBinder onBind(@NonNull Intent intent) {
        Log.d("RevenyInjector", "RootService: onBind");

        Handler handler = new Handler(Looper.getMainLooper(), this);
        return new Messenger(handler).getBinder();
    }

    @Override
    public boolean handleMessage(@NonNull Message message) {
        Log.d("RevenyInjector", "RootService: handleMessage");

        if (message.what != 1) {
            return false;
        }

        Message msg = Message.obtain();
        Bundle bundle = new Bundle();

        String pkgName = message.getData().getString("package_name");
        String libPath = message.getData().getString("library_path");
        String launcherAct = message.getData().getString("launcher_activity");
        boolean autoLaunch = message.getData().getBoolean("auto_launch");
        boolean killBeforeLaunch = message.getData().getBoolean("kill_before_launch");
        boolean remapLibrary = message.getData().getBoolean("remap_library");

        int result = Native.Inject(pkgName, libPath, launcherAct, autoLaunch, killBeforeLaunch, remapLibrary);
        String[] log = Native.GetNativeLogs();

        bundle.putInt("result", result);
        bundle.putStringArray("log", log);

        msg.setData(bundle);

        try {
            message.replyTo.send(msg);
        } catch (RemoteException e) {
            e.printStackTrace();
        }

        return false;
    }
}
