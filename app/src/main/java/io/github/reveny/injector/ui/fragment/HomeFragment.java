package io.github.reveny.injector.ui.fragment;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.text.HtmlCompat;
import androidx.core.view.MenuProvider;
import androidx.fragment.app.DialogFragment;

import java.io.File;

import io.github.reveny.injector.App;
import io.github.reveny.injector.BuildConfig;
import io.github.reveny.injector.R;
import io.github.reveny.injector.databinding.DialogAboutBinding;
import io.github.reveny.injector.databinding.FragmentHomeBinding;
import io.github.reveny.injector.ui.activity.SettingsActivity;
import io.github.reveny.injector.ui.dialog.BlurBehindDialogBuilder;
import io.github.reveny.injector.core.Utility;
import io.github.reveny.injector.util.chrome.LinkTransformationMethod;
import rikka.material.app.LocaleDelegate;

public class HomeFragment extends BaseFragment implements MenuProvider {
    private FragmentHomeBinding binding;
    private final SharedPreferences pref = App.getPreferences();

    @Override
    public void onPrepareMenu(Menu menu) {
        menu.findItem(R.id.menu_settings).setOnMenuItemClickListener(v -> {
            startActivity(new Intent(requireActivity(), SettingsActivity.class));
            return true;
        });
        menu.findItem(R.id.menu_about).setOnMenuItemClickListener(v -> {
            showAbout();
            return true;
        });
    }

    @Override
    public void onCreateMenu(@NonNull Menu menu, @NonNull MenuInflater menuInflater) {

    }

    @Override
    public boolean onMenuItemSelected(@NonNull MenuItem menuItem) {
        return false;
    }

    @SuppressLint("NotifyDataSetChanged")
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        binding = FragmentHomeBinding.inflate(inflater, container, false);
        setupToolbar();
        setupDeviceInfo();
        return binding.getRoot();
    }

    private void setupToolbar() {
        setupToolbar(binding.toolbar, null, R.string.app_name, R.menu.menu_home);
        binding.toolbar.setNavigationIcon(null);
        binding.appBar.setLiftable(true);
        binding.nestedScrollView.getBorderViewDelegate().setBorderVisibilityChangedListener((top, oldTop, bottom, oldBottom) -> binding.appBar.setLifted(!top));
    }

    @SuppressLint("DefaultLocale")
    private void setupDeviceInfo() {
        binding.systemVersion.setText(String.format("Android %s (API %d)", android.os.Build.VERSION.RELEASE, android.os.Build.VERSION.SDK_INT));
        binding.device.setText(String.format("%s %s", Build.BRAND, Build.MODEL));
        binding.systemAbi.setText(android.os.Build.SUPPORTED_ABIS[0]);
        binding.isRooted.setText(Utility.isRooted() ? "Yes" : "No");

        String[] packageNames = {"io.github.huskydg.magisk", "me.weishu.kernelsu", "me.bmax.apatch", "io.github.vvb2060.magisk", "com.topjohnwu.magisk"};
        binding.rootSystem.setText(checkRootSolution(requireContext(), packageNames));
        binding.isEmulator.setText(Utility.isEmulator() ? "Yes" : "No");
    }

    public String checkRootSolution(Context context, String[] packageNames) {
        PackageManager packageManager = context.getPackageManager();
        for (String packageName : packageNames) {
            try {
                PackageInfo packageInfo = packageManager.getPackageInfo(packageName, PackageManager.GET_ACTIVITIES);
                if (packageInfo != null) {
                    return packageManager.getApplicationLabel(packageInfo.applicationInfo).toString();
                }
            } catch (PackageManager.NameNotFoundException e) {
                // Continue checking the next package name
            }
        }

        // If not detected, it's likely that it runs magisk with a spoofed package name
        // We can just check for magisk binary here.
        // TODO: Need to check if the path is still reliable
        if (new File("/sbin/magiskpolicy").exists()) {
            return "Magisk";
        }

        return "Not detected";
    }

    public static class AboutDialog extends DialogFragment {
        @NonNull
        @Override
        public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
            DialogAboutBinding binding = DialogAboutBinding.inflate(getLayoutInflater(), null, false);
            binding.designAboutTitle.setText(R.string.app_name);
            binding.designAboutInfo.setMovementMethod(LinkMovementMethod.getInstance());
            binding.designAboutInfo.setTransformationMethod(new LinkTransformationMethod(requireActivity()));
            binding.designAboutInfo.setText(HtmlCompat.fromHtml(getString(
                    R.string.about_view_source_code,
                    "<b><a href=\"https://github.com/reveny\">GitHub</a></b>",
                    "<b><a href=\"https://t.me/reveny1\">Telegram</a></b>"), HtmlCompat.FROM_HTML_MODE_LEGACY));
            binding.designAboutVersion.setText(String.format(LocaleDelegate.getDefaultLocale(), "%s (%d)", BuildConfig.VERSION_NAME, BuildConfig.VERSION_CODE));
            return new BlurBehindDialogBuilder(requireContext())
                    .setView(binding.getRoot()).create();
        }
    }

    private void showAbout() {
        new AboutDialog().show(getChildFragmentManager(), "about");
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}