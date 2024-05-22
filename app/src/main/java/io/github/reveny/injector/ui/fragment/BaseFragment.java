
package io.github.reveny.injector.ui.fragment;

import android.os.Build;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;

import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.appcompat.widget.Toolbar;
import androidx.core.view.MenuProvider;
import androidx.fragment.app.Fragment;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.fragment.NavHostFragment;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.snackbar.Snackbar;
import com.google.android.material.textfield.TextInputLayout;

import java.util.Objects;

import io.github.reveny.injector.App;
import io.github.reveny.injector.R;

public abstract class BaseFragment extends Fragment {

    public void navigateUp() {
        getNavController().navigateUp();
    }

    public NavController getNavController() {
        return NavHostFragment.findNavController(this);
    }

    public boolean safeNavigate(@IdRes int resId) {
        try {
            getNavController().navigate(resId);
            return true;
        } catch (IllegalArgumentException ignored) {
            return false;
        }
    }

    public void setupToolbar(Toolbar toolbar, View tipsView, int title) {
        setupToolbar(toolbar, tipsView, getString(title), -1);
    }

    public void setupToolbar(Toolbar toolbar, View tipsView, int title, int menu) {
        setupToolbar(toolbar, tipsView, getString(title), menu, null);
    }

    public void setupToolbar(Toolbar toolbar, View tipsView, String title, int menu) {
        setupToolbar(toolbar, tipsView, title, menu, null);
    }

    public void setupToolbar(@NonNull Toolbar toolbar, View tipsView, String title, int menu, View.OnClickListener navigationOnClickListener) {
        toolbar.setNavigationOnClickListener(navigationOnClickListener == null ? (v -> navigateUp()) : navigationOnClickListener);
        toolbar.setTitle(title);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            toolbar.setTooltipText(title);
            if (tipsView != null) tipsView.setTooltipText(title);
        }
        if (menu != -1) {
            toolbar.inflateMenu(menu);
            if (this instanceof MenuProvider self) {
                toolbar.setOnMenuItemClickListener(self::onMenuItemSelected);
                self.onPrepareMenu(toolbar.getMenu());
            }
        }
    }

    public void addTextWatcher(TextInputLayout textInputLayout) {
        Objects.requireNonNull(textInputLayout.getEditText()).addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                textInputLayout.setError(null);
                textInputLayout.setErrorEnabled(false);
            }

            @Override
            public void afterTextChanged(Editable s) {
            }
        });
    }
    public boolean validateAndSetError(TextInputLayout textInputLayout, String errorMessage) {
        if (textInputLayout.isEnabled() && Objects.requireNonNull(textInputLayout.getEditText()).getText().toString().isEmpty()) {
            textInputLayout.setError(errorMessage);
            return false;
        } else {
            textInputLayout.setError(null);
            return true;
        }
    }

}
