package io.github.reveny.injector.ui.fragment;

import android.annotation.SuppressLint;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;

import io.github.reveny.injector.R;
import io.github.reveny.injector.core.LogManager;
import io.github.reveny.injector.databinding.FragmentLogsBinding;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.List;

public class LogsFragment extends BaseFragment {
    private FragmentLogsBinding binding;
    private LogAdapter logAdapter;

    @SuppressLint("NotifyDataSetChanged")
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        binding = FragmentLogsBinding.inflate(inflater, container, false);
        setupToolbar();
        setupRecyclerView();
        return binding.getRoot();
    }

    private void setupToolbar() {
        setupToolbar(binding.toolbar, binding.clickView, R.string.logs);
        binding.toolbar.setNavigationIcon(null);
        binding.appBar.setLiftable(true);
    }

    private void setupRecyclerView() {
        List<String> logs = LogManager.GetLogs();
        logAdapter = new LogAdapter(logs);
        binding.recyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        binding.recyclerView.setAdapter(logAdapter);

        // applyDarkerBackgroundColor();
    }

    private void applyDarkerBackgroundColor() {
        // Get the primary surface color from the current theme
        TypedArray typedArray = requireContext().getTheme().obtainStyledAttributes(new int[]{android.R.attr.colorBackground});
        int color = typedArray.getColor(0, ContextCompat.getColor(requireContext(), android.R.color.background_light));
        typedArray.recycle();

        // Calculate a slightly darker color
        int darkerColor = darkenColor(color, 0.1f);

        // Apply the darker color to the RecyclerView's background
        binding.recyclerView.setBackgroundColor(darkerColor);
    }

    private int darkenColor(int color, float factor) {
        int a = Color.alpha(color);
        int r = Math.round(Color.red(color) * (1 - factor));
        int g = Math.round(Color.green(color) * (1 - factor));
        int b = Math.round(Color.blue(color) * (1 - factor));
        return Color.argb(a, r, g, b);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}
