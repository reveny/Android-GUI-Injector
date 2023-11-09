package com.reveny.injector.v2.ui;

import android.os.Bundle;

import androidx.fragment.app.Fragment;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.reveny.injector.v2.LogManager;
import com.reveny.injector.v2.R;

import java.util.ArrayList;
import java.util.List;

public class LogsFragment extends Fragment {
    private TextView console;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View root = inflater.inflate(R.layout.fragment_logs, container, false);
        console = root.findViewById(R.id.console);

        return root;
    }

    @Override
    public void onResume() {
        super.onResume();

        // Executed every time Log View gets opened
        console.setText(""); // clear console

        // Load Logs again
        for (String l : LogManager.GetLogs()) {
            console.append(l + "\n");
        }
    }
}