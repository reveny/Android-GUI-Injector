package io.github.reveny.injector.ui.activity;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.fragment.NavHostFragment;
import androidx.navigation.ui.NavigationUI;

import com.google.android.material.navigation.NavigationBarView;

import io.github.reveny.injector.R;
import io.github.reveny.injector.databinding.ActivityMainBinding;

public class MainActivity extends BaseActivity {
    private static final String KEY_PREFIX = MainActivity.class.getName() + '.';
    private static final String EXTRA_SAVED_INSTANCE_STATE = KEY_PREFIX + "SAVED_INSTANCE_STATE";
    private boolean restarting;
    private ActivityMainBinding binding;

    @NonNull
    public static Intent newIntent(@NonNull Context context) {
        return new Intent(context, MainActivity.class);
    }

    @NonNull
    private static Intent newIntent(@NonNull Bundle savedInstanceState, @NonNull Context context) {
        return newIntent(context)
                .putExtra(EXTRA_SAVED_INSTANCE_STATE, savedInstanceState);
    }

    @Override
    public boolean onSupportNavigateUp() {
        NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment);
        return navController.navigateUp() || super.onSupportNavigateUp();
    }


    @Override
    public void onCreate(Bundle savedInstanceState) {
        if (savedInstanceState == null) {
            savedInstanceState = getIntent().getBundleExtra(EXTRA_SAVED_INSTANCE_STATE);
        }
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        NavHostFragment navHostFragment = (NavHostFragment) getSupportFragmentManager().findFragmentById(R.id.nav_host_fragment);
        if (navHostFragment == null) {
            return;
        }

        NavController navController = navHostFragment.getNavController();
        var nav = (NavigationBarView) binding.nav;
        NavigationUI.setupWithNavController(nav, navController);
    }

}