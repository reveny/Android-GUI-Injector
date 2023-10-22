package com.reveny.injector.v2.ui;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Build;
import android.os.Bundle;

import androidx.fragment.app.Fragment;

import android.text.InputType;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.TextView;

import com.reveny.injector.v2.LogManager;
import com.reveny.injector.v2.R;
import com.reveny.injector.v2.Utility;
import com.reveny.injector.v2.root.InjectorData;
import com.reveny.injector.v2.root.RootManager;
import com.reveny.injector.v2.root.services.RootHandler;
import com.reveny.injector.v2.root.services.RootService;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class InjectFragment extends Fragment {
    public static InjectFragment instance;

    // App List
    private AutoCompleteTextView autoCompleteTextView;
    private ArrayAdapter<String> adapterItems;

    // App Data
    private TextView archText;
    private TextView pidText;

    // Injection Method
    private AutoCompleteTextView injectionAutoCompleteTextView;
    private ArrayAdapter<String> injectionAdapterItems;
    private String[] injectionMethods = {"Ptrace"};

    // Injection Path
    private Button changeLibPathBtn;
    private TextView libPathText;


    // Injector Data
    public InjectorData injectorData = new InjectorData();

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View root = inflater.inflate(R.layout.fragment_inject, container, false);
        instance = this;

        autoCompleteTextView = root.findViewById(R.id.app_selector_text);

        archText = root.findViewById(R.id.architecture_text);
        pidText = root.findViewById(R.id.pid_text);

        injectionAutoCompleteTextView = root.findViewById(R.id.injection_method_text);

        changeLibPathBtn = root.findViewById(R.id.change_path_btn);
        libPathText = root.findViewById(R.id.path_text);

        changeLibPathBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ShowChangePathMessageBox();
            }
        });

        injectorData.libraryPath = "/data/local/tmp/libTest.so";

        SetDeviceInfo(root);
        SetInstalledApps();
        SetAutoLaunchOptions(root);
        SetInjectionMethods();
        SetSettings(root);
        SetButtons(root);

        return root;
    }

    @SuppressLint("SetTextI18n")
    private void SetDeviceInfo(View root) {
        TextView androidVersion = root.findViewById(R.id.android_version_text);
        TextView isRooted = root.findViewById(R.id.rooted_text);
        TextView isEmulator = root.findViewById(R.id.emulator_text);
        TextView arch = root.findViewById(R.id.arch_text);

        androidVersion.setText("Android: " + Build.VERSION.RELEASE);
        isRooted.setText("Rooted: " + Utility.isRooted());
        isEmulator.setText("Emulator: " + Utility.isEmulator());
        arch.setText("Architecture: " + System.getProperty("os.arch"));
    }

    private void SetInstalledApps() {
        adapterItems = new ArrayAdapter<String>(requireActivity(), R.layout.list_item, getInstalledApps());
        autoCompleteTextView.setAdapter(adapterItems);
        autoCompleteTextView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @SuppressLint("SetTextI18n")
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String item = parent.getItemAtPosition(position).toString();
                injectorData.packageName = item;

                // Set launch activity
                injectorData.launcherActivity = getLaunchActivity(item);

                String arch = "Unknown";
                try {
                    String libraryDir = requireActivity().getPackageManager().getApplicationInfo(item, 0).nativeLibraryDir;
                    arch = libraryDir.substring(libraryDir.lastIndexOf("/") + 1);
                } catch (PackageManager.NameNotFoundException exception) {
                    exception.printStackTrace();
                }
                archText.setText("Architecture: " + arch);

                boolean running = RootManager.instance.isAppRunning(item);
                if (running) {
                    pidText.setText("PID: " + RootManager.instance.getPid(item));
                } else {
                    pidText.setText("PID: No Running (-1)");
                }

                LogManager.AddLog("Selected Package Name: " + item);
            }
        });
    }

    private void SetAutoLaunchOptions(View root) {
        CheckBox autoLaunch = root.findViewById(R.id.auto_launch);
        CheckBox killBeforeLaunch = root.findViewById(R.id.kill_proc);

        autoLaunch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                injectorData.shouldAutoLaunch = b;
            }
        });

        killBeforeLaunch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                if (b) {
                    autoLaunch.setChecked(true);
                }

                injectorData.shouldKillBeforeLaunch = b;
            }
        });
    }

    private void SetInjectionMethods() {
        injectionAdapterItems = new ArrayAdapter<String>(requireActivity(), R.layout.list_item, injectionMethods);
        injectionAutoCompleteTextView.setAdapter(injectionAdapterItems);
    }

    private void ShowChangePathMessageBox() {
        AlertDialog.Builder builder = new AlertDialog.Builder(requireActivity());
        builder.setTitle("Enter library path");

        EditText input = new EditText(requireActivity());
        input.setText(injectorData.libraryPath);
        input.setInputType(InputType.TYPE_CLASS_TEXT);
        builder.setView(input);

        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                injectorData.libraryPath = input.getText().toString();
                libPathText.setText(input.getText());
            }
        });
        builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.cancel();
            }
        });

        builder.show();
    }

    private void SetSettings(View root) {
        CheckBox remapLibrary = root.findViewById(R.id.remap_lib);

        remapLibrary.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                injectorData.remapLibrary = b;
            }
        });
    }

    private void SetButtons(View view) {
        Button injectButton = view.findViewById(R.id.inject_button);
        Button githubButton = view.findViewById(R.id.github_button);

        injectButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                LogManager.AddLog("Starting Injection with following settings: ");
                LogManager.AddLog("--------------------------------------------");
                LogManager.AddLog("package_name:        " + injectorData.packageName);
                LogManager.AddLog("library_path:        " + injectorData.libraryPath);
                LogManager.AddLog("launcher_activity:   " + injectorData.launcherActivity);
                LogManager.AddLog("auto_launch:         " + injectorData.shouldAutoLaunch);
                LogManager.AddLog("kill_before_launch:  " + injectorData.shouldKillBeforeLaunch);
                LogManager.AddLog("remap_library:       " + injectorData.remapLibrary);
                LogManager.AddLog("--------------------------------------------");

                RootHandler.instance.Inject(requireActivity());
            }
        });

        githubButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Utility.OpenURL(requireActivity(), "https://github.com/reveny");
            }
        });
    }

    private List<String> getInstalledApps() {
        List<ApplicationInfo> packages = requireActivity().getPackageManager().getInstalledApplications(PackageManager.GET_META_DATA);
        List<String> ret = new ArrayList<String>();

        for (ApplicationInfo s : packages) {
            //Filter system apps and this app
            if (s.sourceDir.startsWith("/data") && !s.sourceDir.contains("com.reveny.injector") ) {
                ret.add(s.packageName);
            }
        }

        return ret;
    }

    private String getLaunchActivity(String packageName) {
        String activityName = "";
        final PackageManager pm = requireActivity().getPackageManager();
        Intent intent = pm.getLaunchIntentForPackage(packageName);
        if (intent != null) {
            List<ResolveInfo> activityList = pm.queryIntentActivities(intent,0);
            activityName = activityList.get(0).activityInfo.name;
            return packageName + "/" + activityName;
        }
        return "";
    }
}