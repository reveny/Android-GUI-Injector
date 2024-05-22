package io.github.reveny.injector.ui.fragment;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.List;

import io.github.reveny.injector.R;

public class LogAdapter extends RecyclerView.Adapter<LogAdapter.LogViewHolder> {

    private List<String> logList;

    public LogAdapter(List<String> logList) {
        this.logList = logList;
    }

    @NonNull
    @Override
    public LogViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_log, parent, false);
        return new LogViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull LogViewHolder holder, int position) {
        holder.logTextView.setText(logList.get(position));
    }

    @Override
    public int getItemCount() {
        return logList.size();
    }

    public static class LogViewHolder extends RecyclerView.ViewHolder {
        TextView logTextView;

        public LogViewHolder(@NonNull View itemView) {
            super(itemView);
            logTextView = itemView.findViewById(R.id.log_text);
        }
    }
}
