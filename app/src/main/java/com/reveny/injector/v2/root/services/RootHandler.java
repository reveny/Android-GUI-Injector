package com.reveny.injector.v2.root.services;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

import androidx.annotation.NonNull;

import com.reveny.injector.v2.LogManager;
import com.reveny.injector.v2.ui.InjectFragment;

public class RootHandler implements Handler.Callback {
    public static RootHandler instance = new RootHandler();

    //Root connection
    private MSGConnection messageConnection;
    private Messenger remoteMessenger;
    private final Messenger replyMessenger = new Messenger(new Handler(Looper.getMainLooper(), this));

    public void Inject(Activity activity) {
        MSGConnection mSGConnection = messageConnection;
        if (mSGConnection == null) {
            RootService.bind(new Intent(activity, RootService.class), new MSGConnection());
        } else {
            RootService.unbind(mSGConnection);
        }
    }

    @Override
    public boolean handleMessage(@NonNull Message message) {
        int result = message.getData().getInt("result");

        if (result == -1) {
            LogManager.AddLog("Injection failed: " + result);
        } else if (result == 0) {
            LogManager.AddLog("Injection success: " + result);
        } else {
            LogManager.AddLog("Something went completely wrong: " + result);
        }

        return false;
    }

    public class MSGConnection implements ServiceConnection {
        MSGConnection() {}

        @Override
        public void onServiceConnected(ComponentName componentName, IBinder iBinder) {
            Log.d("RevenyInjector", "MSGConnection: onServiceConnected");
            remoteMessenger = new Messenger(iBinder);
            messageConnection = this;

            Message message = Message.obtain((Handler) null, 1);
            message.getData().putString("package_name", InjectFragment.instance.injectorData.packageName);
            message.getData().putString("library_path", InjectFragment.instance.injectorData.libraryPath);
            message.getData().putString("launcher_activity", InjectFragment.instance.injectorData.launcherActivity);
            message.getData().putBoolean("auto_launch", InjectFragment.instance.injectorData.shouldAutoLaunch);
            message.getData().putBoolean("kill_before_launch", InjectFragment.instance.injectorData.shouldKillBeforeLaunch);
            message.getData().putBoolean("remap_library", InjectFragment.instance.injectorData.remapLibrary);
            message.replyTo = replyMessenger;

            try {
                remoteMessenger.send(message);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            Log.d("RevenyInjector", "MSGConnection: onServiceDisconnected");
            remoteMessenger = null;
            messageConnection = null;
        }
    }
}
