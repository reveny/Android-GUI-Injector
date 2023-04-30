package com.reveny.injector;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.button.MaterialButton;
import com.topjohnwu.superuser.Shell;

import android.annotation.SuppressLint;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;
import android.util.StateSet;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends AppCompatActivity implements Handler.Callback {
    private MainActivity thisInstance;

    //UI
    AutoCompleteTextView autoCompleteTextView;
    EditText libPath;

    EditText functionName;
    CheckBox autoLaunchBox;
    CheckBox ptraceBox;
    CheckBox ldpreloadBox;
    TextView archText;
    TextView console;
    Button githubButton;
    Button uninjectButton;
    Button injectButton;

    ArrayAdapter<String> adapterItems;

    public String packageName = "";
    public String finalLibPath = "";
    public String finalFunctionName = "";
    public String launchActivity;
    public boolean shouldAutoLaunch = true;

    //Root connection
    private MSGConnection messageConnection;
    private Messenger remoteMessenger;
    private final Messenger replyMessenger = new Messenger(new Handler(Looper.getMainLooper(), this));

    private boolean hasRootAccess = false;

    //Setup libsu
    static {
        Shell.enableVerboseLogging = true;
        Shell.setDefaultBuilder(Shell.Builder.create()
                .setFlags(Shell.FLAG_REDIRECT_STDERR)
                .setTimeout(10)
        );
    }

    @SuppressLint("SetTextI18n")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        thisInstance = this;

        autoCompleteTextView = findViewById(R.id.auto_complete_txt);
        libPath = findViewById(R.id.path_to_lib);
        functionName = findViewById(R.id.function_name);
        githubButton = findViewById(R.id.github_button);
        uninjectButton = findViewById(R.id.tutorial_button);
        injectButton = findViewById(R.id.inject_button);
        autoLaunchBox = findViewById(R.id.auto_launch_toggle);
        ptraceBox = findViewById(R.id.mode_ptrace);
        ldpreloadBox = findViewById(R.id.mode_ldpreload);
        archText = findViewById(R.id.architecture);
        console = findViewById(R.id.console);

        //Set installed packages
        adapterItems = new ArrayAdapter<String>(this, R.layout.list_item, getInstalledApps());
        autoCompleteTextView.setAdapter(adapterItems);
        autoCompleteTextView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String item = parent.getItemAtPosition(position).toString();
                packageName = item;
                console.append("Package Name: " + item + "\n");
                String arch = "Unknown";

                try {
                    String libraryDir = getPackageManager().getApplicationInfo(packageName, 0).nativeLibraryDir;
                    arch = libraryDir.substring(libraryDir.lastIndexOf("/") + 1);
                } catch (PackageManager.NameNotFoundException exception) {
                    exception.printStackTrace();
                }

                archText.setText("Architecture: " + arch);
            }
        });

        libPath.setText("/data/local/tmp/libnative.so"); //Set default path
        console.append("Device Architecture: " + Build.CPU_ABI.toString() + " \n");


        injectButton.setOnClickListener(new View.OnClickListener()  {
            @Override
            public void onClick(View v) {
                checkLibPath();
                if (hasRootAccess) {
                    if (packageName.equals("") || finalLibPath.equals("")) {
                        console.append("Please fill out all the fields\n");
                    } else {
                        if (ptraceBox.isChecked()) {
                            MSGConnection mSGConnection = messageConnection;
                            if (mSGConnection == null) {
                                console.append("Binding root services\n");

                                finalFunctionName = functionName.getText().toString();
                                shouldAutoLaunch = autoLaunchBox.isChecked();
                                launchActivity = getLaunchActivity(packageName);

                                console.append("---------------------------------------\n");
                                console.append("Trying to Inject with following settings: \n");
                                console.append("shouldAutoLaunch: " + shouldAutoLaunch + "\n");
                                console.append("packageName: " + packageName + "\n");
                                console.append("launchActivity: " + launchActivity + "\n");
                                console.append("finalLibPath: " + finalLibPath + "\n");
                                console.append("functionName: " + finalFunctionName + "\n");
                                console.append("---------------------------------------\n");
                                RootService.bind(new Intent(thisInstance, RootService.class), new MSGConnection());
                            } else {
                                RootService.unbind(mSGConnection);
                            }
                        } else if (ldpreloadBox.isChecked()) {
                            if (packageName.equals("") || finalLibPath.equals("")) {
                                console.append("Please fill out all the fields\n");
                            } else {
                                String command = "setprop wrap." + packageName + " LD_PRELOAD=" + finalLibPath;
                                Shell.cmd(command).exec();
                                Toast.makeText(thisInstance, "Injected! The game might take longer to load", Toast.LENGTH_LONG).show();
                            }
                        } else {
                            console.append("You need to select an Injection mode \n");
                        }
                    }
                } else {
                    console.append("Bind root service failed: root access not granted\n");
                }
            }
        });

        uninjectButton.setOnClickListener(new View.OnClickListener()  {
            @Override
            public void onClick(View v) {
                if (packageName.isEmpty()) {
                    console.append("Cannot uninject without a package name\n");
                } else {
                    String command = "resetprop --delete wrap." + packageName;
                    Shell.cmd(command).exec();
                    Toast.makeText(thisInstance, "Uninjected!", Toast.LENGTH_LONG).show();
                }
            }
        });

        githubButton.setOnClickListener(new View.OnClickListener()  {
            @Override
            public void onClick(View v) {
                Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://github.com/reveny"));
                startActivity(browserIntent);
            }
        });

        //Root perm window
        Shell.getShell(shell -> {
            console.append("Injector launched\n");
            hasRootAccess = true;
        });
    }

    @Override
    public boolean handleMessage(@NonNull Message message) {
        int result = message.getData().getInt("result");

        if (result == -1) {
            console.append("Injection failed\n");
        } else if (result == 0) {
            console.append("Injection success\n");
        } else {
            console.append("Something went completely wrong\n");
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
            message.getData().putString("pkg", packageName);
            message.getData().putString("lib", finalLibPath);
            message.getData().putString("fnc", finalFunctionName);
            message.getData().putString("launcherAct", launchActivity);
            message.getData().putBoolean("launch", shouldAutoLaunch);
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

    public List<String> getInstalledApps() {
        List<ApplicationInfo> packages = getPackageManager().getInstalledApplications(PackageManager.GET_META_DATA);
        List<String> ret = new ArrayList<String>();

        for (ApplicationInfo s : packages) {
            //Filter system apps and this app
            if (s.sourceDir.startsWith("/data") && !s.sourceDir.contains("com.reveny.injector") ) {
                ret.add(s.packageName);
            }
        }

        return ret;
    }

    private void checkLibPath() {
        String path = libPath.getText().toString();
        File file = new File(path);
        finalLibPath = "/data/local/tmp/" + file.getName();

        //Check if lib is in /data/local/tmp
        if (!path.startsWith("/data/local/tmp")) {
            //File is not in /data/local/tmp so we need to copy it there
            String cmd = "cp " + path + " /data/local/tmp/" + file.getName();
            Shell.cmd(cmd).exec();
            finalLibPath = "/data/local/tmp/" + file.getName();
        }
    }

    private String getLaunchActivity(String packageName) {
        String activityName = "";
        final PackageManager pm = getPackageManager();
        Intent intent = pm.getLaunchIntentForPackage(packageName);
        List<ResolveInfo> activityList = pm.queryIntentActivities(intent,0);
        if(activityList != null) {
            activityName = activityList.get(0).activityInfo.name;
        }
        return packageName + "/" + activityName;
    }
}