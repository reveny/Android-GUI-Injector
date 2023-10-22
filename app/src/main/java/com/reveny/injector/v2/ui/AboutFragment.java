package com.reveny.injector.v2.ui;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import androidx.fragment.app.Fragment;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.reveny.injector.v2.R;
import com.reveny.injector.v2.Utility;

public class AboutFragment extends Fragment {

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View root = inflater.inflate(R.layout.fragment_about, container, false);

        Button githubButton = root.findViewById(R.id.github_button);
        Button telegramButton = root.findViewById(R.id.telegram_button);
        Button telegramChannelButton = root.findViewById(R.id.telegram_channel_button);

        githubButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Utility.OpenURL(requireActivity(), "https://github.com/reveny");
            }
        });

        telegramButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Utility.OpenURL(requireActivity(), "https://t.me/revenyy");
            }
        });

        telegramChannelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Utility.OpenURL(requireActivity(), "https://t.me/reveny1");
            }
        });

        return root;
    }
}