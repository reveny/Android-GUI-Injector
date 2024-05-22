package io.github.reveny.injector.ui.fragment;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.net.URI;
import java.nio.file.Paths;
import java.util.Objects;

import io.github.reveny.injector.App;
import io.github.reveny.injector.R;
import io.github.reveny.injector.core.InjectorData;
import io.github.reveny.injector.core.LogManager;
import io.github.reveny.injector.core.root.RootHandler;
import io.github.reveny.injector.core.root.RootManager;
import io.github.reveny.injector.databinding.FragmentInjectionBinding;
import io.github.reveny.injector.core.Utility;

public class InjectionFragment extends BaseFragment {
    private FragmentInjectionBinding binding;

    public InjectorData injectorData = new InjectorData();

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode != 1 || resultCode != Activity.RESULT_OK) {
            return;
        }

        if (data == null || data.getData() == null) {
            Toast.makeText(getActivity(), "File selection failed", Toast.LENGTH_SHORT).show();
            return;
        }

        // I don't think this is the best way of converting the URI to a path, but it works for now
        Uri fileUri = data.getData();
        String path = Objects.requireNonNull(fileUri.getPath()).replace("/document/primary:", Environment.getExternalStorageDirectory().getPath() + "/");

        if (path.endsWith(".so") || path.endsWith(".dex")) {
            Toast.makeText(getActivity(), "File Selected: " + path, Toast.LENGTH_LONG).show();

            injectorData.setLibraryPath(path);
            binding.libPath.setText(path);
        } else {
            Toast.makeText(getActivity(), "Invalid file type selected. Please select a .so or .dex file.", Toast.LENGTH_SHORT).show();
        }
    }

    @SuppressLint("NotifyDataSetChanged")
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        binding = FragmentInjectionBinding.inflate(inflater, container, false);

        setupToolbar();
        setupApplist();
        setupAutoLaunch();
        setupSettings();

        binding.libPathChoose.setEndIconOnClickListener(v -> {
            Intent chooseFile = new Intent(Intent.ACTION_GET_CONTENT);
            chooseFile.setType("*/*");

            // For .so and .dex files
            chooseFile.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"application/octet-stream"});
            chooseFile = Intent.createChooser(chooseFile, "Choose a .so or .dex file");
            startActivityForResult(chooseFile, 1);
        });

        binding.injectionFab.setOnClickListener(v -> {
            addTextWatcher(binding.appSelector);
            addTextWatcher(binding.libPathChoose);
            if (validateInputs(binding)) {
                startInjection();
            }
        });
        return binding.getRoot();
    }

    private void setupToolbar() {
        setupToolbar(binding.toolbar, binding.clickView, R.string.injection);
        binding.toolbar.setNavigationIcon(null);
        binding.appBar.setLiftable(true);
        binding.nestedScrollView.getBorderViewDelegate().setBorderVisibilityChangedListener((top, oldTop, bottom, oldBottom) -> binding.appBar.setLifted(!top));
    }

    private void setupApplist() {
        ArrayAdapter<String> adapter = new ArrayAdapter<>(
            requireContext(),
            android.R.layout.simple_dropdown_item_1line,
            Utility.getInstalledApps(requireContext())
        );
        binding.appSelectorText.setAdapter(adapter);

        binding.appSelectorText.setOnItemClickListener((parent, view, position, id) -> {
            String selected = (String) parent.getItemAtPosition(position);

            injectorData.setPackageName(selected);
            injectorData.setLauncherActivity(Utility.getLaunchActivity(requireContext(), selected));

            String processID = RootManager.instance.getPid(selected);
            binding.killProc.setText(String.format("Kill before Auto Launch [PID: %s]", processID));

            LogManager.AddLog("Selected: " + selected);
        });
    }

    private void setupAutoLaunch() {
        binding.autoLaunch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            injectorData.setShouldAutoLaunch(isChecked);
            LogManager.AddLog("Auto Launch: " + isChecked);
        });

        binding.killProc.setOnCheckedChangeListener((buttonView, isChecked) -> {
            injectorData.setShouldKillBeforeLaunch(isChecked);
            LogManager.AddLog("Kill Process: " + isChecked);
        });
    }

    private void setupSettings() {
        binding.remapLib.setOnCheckedChangeListener((buttonView, isChecked) -> {
            injectorData.setRemapLibrary(isChecked);
            LogManager.AddLog("Remap Library: " + isChecked);
        });

        binding.enableProxy.setOnCheckedChangeListener((buttonView, isChecked) -> {
            injectorData.setUseProxy(isChecked);
            LogManager.AddLog("Use Proxy: " + isChecked);
        });

        binding.enableProxyRandomize.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) binding.enableProxy.setChecked(true);

            injectorData.setRandomizeProxyName(isChecked);
            LogManager.AddLog("Randomize Proxy: " + isChecked);
        });

        /*
        binding.copyToCache.setOnCheckedChangeListener((buttonView, isChecked) -> {
            injectorData.setCopyToCache(isChecked);
            LogManager.AddLog("Copy to Cache: " + isChecked);
        });
         */

        binding.hideLibrary.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) binding.enableProxy.setChecked(true);

            injectorData.setHideLibrary(isChecked);
            LogManager.AddLog("Hide Library: " + isChecked);
        });

        binding.bypassRestrictions.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) binding.enableProxy.setChecked(true);

            if (Utility.isEmulator()) {
                Toast.makeText(requireContext(), "Emulator detected, bypassing restrictions does not work here!", Toast.LENGTH_SHORT).show();
                if (isChecked) binding.bypassRestrictions.setChecked(false);
                return;
            }

            injectorData.setBypassNamespaceRestrictions(isChecked);
            LogManager.AddLog("Bypass Restrictions: " + isChecked);
        });
    }

    private void startInjection() {
        LogManager.AddLog("Starting Injection...");
        LogManager.AddLog("Injection Data: " + injectorData.toString());

        // Make sure we have root access
        if (!RootManager.instance.hasRootAccess) {
            Toast.makeText(requireContext(), "Root Access not granted, please restart the app", Toast.LENGTH_SHORT).show();
            return;
        }

        // Start the injection process
        RootHandler rootHandler = new RootHandler();
        rootHandler.Inject(requireActivity());
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }

    private boolean validateInputs(FragmentInjectionBinding injectionBinding) {
        boolean isValid = validateAndSetError(injectionBinding.appSelector, "Please select a target package");
        isValid &= validateAndSetError(injectionBinding.libPathChoose, "Please select a library path");
        return isValid;
    }
}
