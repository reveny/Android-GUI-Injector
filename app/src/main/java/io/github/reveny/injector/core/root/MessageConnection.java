package io.github.reveny.injector.core.root;

import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;

import io.github.reveny.injector.core.InjectorData;
import io.github.reveny.injector.core.LogManager;

public class MessageConnection implements ServiceConnection {
    private final RootHandler handler;

    public MessageConnection(RootHandler handler) {
        LogManager.AddLog("MessageConnection: Constructor");
        this.handler = handler;
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        LogManager.AddLog("MessageConnection: onServiceConnected");

        handler.remoteMessenger = new Messenger(service);
        handler.messageConnection = this;

        Message message = InjectorData.instance.toMessage(1);
        message.replyTo = handler.replyMessenger;

        try {
            handler.remoteMessenger.send(message);
        } catch (Exception e) {
            LogManager.AddLog("Failed to send message: " + e.getMessage());
        }
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        LogManager.AddLog("MessageConnection: onServiceDisconnected");

        handler.remoteMessenger = null;
        handler.messageConnection = null;
    }
}
