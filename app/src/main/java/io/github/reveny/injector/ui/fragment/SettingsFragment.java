package io.github.reveny.injector.ui.fragment;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.PowerManager;
import android.provider.Settings;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.activity.OnBackPressedCallback;
import androidx.activity.OnBackPressedDispatcher;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.text.HtmlCompat;
import androidx.navigation.Navigation;
import androidx.navigation.fragment.NavHostFragment;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.color.DynamicColors;

import java.util.ArrayList;
import java.util.Locale;

import io.github.reveny.injector.App;
import io.github.reveny.injector.R;
import io.github.reveny.injector.core.spoof.PackageNameSpoof;
import io.github.reveny.injector.databinding.FragmentSettingsBinding;
import io.github.reveny.injector.ui.activity.SettingsActivity;
import io.github.reveny.injector.util.LangList;
import io.github.reveny.injector.util.NavUtil;
import io.github.reveny.injector.util.ThemeUtil;
import rikka.core.util.ResourceUtils;
import rikka.material.app.LocaleDelegate;
import rikka.material.preference.MaterialSwitchPreference;
import rikka.preference.SimpleMenuPreference;
import rikka.recyclerview.RecyclerViewKt;
import rikka.widget.borderview.BorderRecyclerView;

public class SettingsFragment extends BaseFragment {

    private FragmentSettingsBinding binding;

    @SuppressLint("StaticFieldLeak")

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        binding = FragmentSettingsBinding.inflate(inflater, container, false);
        binding.appBar.setLiftable(true);
        binding.toolbar.setNavigationIcon(R.drawable.arrow_left_outline);
        setupToolbar(binding.toolbar, binding.clickView, R.string.Settings);
        binding.toolbar.setNavigationOnClickListener(v -> requireActivity().getOnBackPressedDispatcher().onBackPressed());

        if (savedInstanceState == null) {
            getChildFragmentManager().beginTransaction().add(R.id.setting_container, new PreferenceFragment()).commitNow();
        }
        super.onCreate(savedInstanceState);
        return binding.getRoot();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }

    public static class PreferenceFragment extends PreferenceFragmentCompat {
        private SettingsFragment parentFragment;

        @Override
        public void onAttach(@NonNull Context context) {
            super.onAttach(context);
            parentFragment = (SettingsFragment) requireParentFragment();
        }

        @Override
        public void onDetach() {
            super.onDetach();
            parentFragment = null;
        }

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            final String SYSTEM = "SYSTEM";

            addPreferencesFromResource(R.xml.prefs);

            Preference theme = findPreference("dark_theme");
            if (theme != null) {
                theme.setOnPreferenceChangeListener((preference, newValue) -> {
                    if (!App.getPreferences().getString("dark_theme", ThemeUtil.MODE_NIGHT_FOLLOW_SYSTEM).equals(newValue)) {
                        AppCompatDelegate.setDefaultNightMode(ThemeUtil.getDarkTheme((String) newValue));
                    }
                    return true;
                });
            }

            MaterialSwitchPreference black_dark_theme = findPreference("black_dark_theme");
            if (black_dark_theme != null) {
                black_dark_theme.setOnPreferenceChangeListener((preference, newValue) -> {
                    SettingsActivity activity = (SettingsActivity) getActivity();
                    if (activity != null && ResourceUtils.isNightMode(getResources().getConfiguration())) {
                        activity.restart();
                    }
                    return true;
                });
            }

            Preference primary_color = findPreference("theme_color");
            if (primary_color != null) {
                primary_color.setOnPreferenceChangeListener((preference, newValue) -> {
                    SettingsActivity activity = (SettingsActivity) getActivity();
                    if (activity != null) {
                        activity.restart();
                    }
                    return true;
                });
            }

            MaterialSwitchPreference prefFollowSystemAccent = findPreference("follow_system_accent");
            if (prefFollowSystemAccent != null && DynamicColors.isDynamicColorAvailable()) {
                if (primary_color != null) {
                    primary_color.setVisible(!prefFollowSystemAccent.isChecked());
                }
                prefFollowSystemAccent.setVisible(true);
                prefFollowSystemAccent.setOnPreferenceChangeListener((preference, newValue) -> {
                    SettingsActivity activity = (SettingsActivity) getActivity();
                    if (activity != null) {
                        activity.restart();
                    }
                    return true;
                });
            }

            SimpleMenuPreference language = findPreference("language");
            if (language != null) {
                var tag = language.getValue();
                var userLocale = App.getLocale();
                var entries = new ArrayList<CharSequence>();
                var lstLang = LangList.LOCALES;
                for (var lang : lstLang) {
                    if (lang.equals(SYSTEM)) {
                        entries.add(getString(rikka.core.R.string.follow_system));
                        continue;
                    }
                    var locale = Locale.forLanguageTag(lang);
                    entries.add(HtmlCompat.fromHtml(locale.getDisplayName(locale), HtmlCompat.FROM_HTML_MODE_LEGACY));
                }
                language.setEntries(entries.toArray(new CharSequence[0]));
                language.setEntryValues(lstLang);
                if (TextUtils.isEmpty(tag) || SYSTEM.equals(tag)) {
                    language.setSummary(getString(rikka.core.R.string.follow_system));
                } else {
                    var locale = Locale.forLanguageTag(tag);
                    language.setSummary(!TextUtils.isEmpty(locale.getScript()) ? locale.getDisplayScript(userLocale) : locale.getDisplayName(userLocale));
                }
                language.setOnPreferenceChangeListener((preference, newValue) -> {
                    var app = App.getInstance();
                    var locale = App.getLocale((String) newValue);
                    var res = app.getResources();
                    var config = res.getConfiguration();
                    config.setLocale(locale);
                    LocaleDelegate.setDefaultLocale(locale);
                    res.updateConfiguration(config, res.getDisplayMetrics());
                    SettingsActivity activity = (SettingsActivity) getActivity();
                    if (activity != null) {
                        activity.restart();
                    }
                    return true;
                });
            }

            Preference randomizePackageName = findPreference("randomize_package_name");
            if (randomizePackageName != null) {
                randomizePackageName.setOnPreferenceClickListener(preference -> {
                    PackageNameSpoof randomize = new PackageNameSpoof();
                    randomize.spoofPackageName();

                    return true;
                });
            }
        }

        @NonNull
        @Override
        public RecyclerView onCreateRecyclerView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent, Bundle savedInstanceState) {
            BorderRecyclerView recyclerView = (BorderRecyclerView) super.onCreateRecyclerView(inflater, parent, savedInstanceState);
            RecyclerViewKt.fixEdgeEffect(recyclerView, false, true);
            recyclerView.getBorderViewDelegate().setBorderVisibilityChangedListener((top, oldTop, bottom, oldBottom) -> parentFragment.binding.appBar.setLifted(!top));
            var fragment = getParentFragment();
            if (fragment instanceof SettingsFragment settingsFragment) {
                View.OnClickListener l = v -> {
                    settingsFragment.binding.appBar.setExpanded(true, true);
                    recyclerView.smoothScrollToPosition(0);
                };
                settingsFragment.binding.toolbar.setOnClickListener(l);
                settingsFragment.binding.clickView.setOnClickListener(l);
            }
            return recyclerView;
        }
    }
}