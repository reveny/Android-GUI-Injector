package com.reveny.injector;

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

        String pkgName = message.getData().getString("pkg");
        String libPath = message.getData().getString("lib");
        String functionName = message.getData().getString("fnc");
        String launcherAct = message.getData().getString("launcherAct");
        boolean autoLaunch = message.getData().getBoolean("launch");

        bundle.putString("fnc", functionName );
        int result = Native.Inject(pkgName, libPath, functionName, launcherAct, autoLaunch);
        bundle.putInt("result", result);
        msg.setData(bundle);

        try {
            message.replyTo.send(msg);
        } catch (RemoteException e) {
            e.printStackTrace();
        }

        return false;
    }
}
