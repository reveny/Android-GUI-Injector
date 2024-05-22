package io.github.reveny.injector.ui.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.text.Layout;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.util.AttributeSet;
import android.util.DisplayMetrics;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.ConcatAdapter;

import io.github.reveny.injector.R;
import io.github.reveny.injector.util.SimpleStatefulAdaptor;
import rikka.core.util.ResourceUtils;

public class EmptyStateRecyclerView extends StatefulRecyclerView {
    private final TextPaint paint = new TextPaint(Paint.ANTI_ALIAS_FLAG);
    private final String emptyText;

    public EmptyStateRecyclerView(Context context) {
        this(context, null);
    }

    public EmptyStateRecyclerView(Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public EmptyStateRecyclerView(Context context, @Nullable AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        DisplayMetrics dm = context.getResources().getDisplayMetrics();

        paint.setColor(ResourceUtils.resolveColor(context.getTheme(), android.R.attr.textColorSecondary));
        paint.setTextSize(16f * dm.scaledDensity);

        emptyText = "Nothing found here";
    }

    @Override
    protected void dispatchDraw(@NonNull Canvas canvas) {
        super.dispatchDraw(canvas);
        var adapter = getAdapter();
        if (adapter instanceof ConcatAdapter) {
            for (var a : ((ConcatAdapter) adapter).getAdapters()) {
                if (a instanceof EmptyStateAdapter) {
                    adapter = a;
                    break;
                }
            }
        }
        if (adapter instanceof EmptyStateAdapter && ((EmptyStateAdapter<?>) adapter).isLoaded() && adapter.getItemCount() == 0) {
            final int width = getMeasuredWidth() - getPaddingLeft() - getPaddingRight();
            final int height = getMeasuredHeight() - getPaddingTop() - getPaddingBottom();

            var textLayout = new StaticLayout(emptyText, paint, width, Layout.Alignment.ALIGN_CENTER, 1.0f, 0.0f, false);

            canvas.save();
            canvas.translate(getPaddingLeft(), (height >> 1) + getPaddingTop() - (textLayout.getHeight() >> 1));

            textLayout.draw(canvas);

            canvas.restore();
        }
    }

    public abstract static class EmptyStateAdapter<T extends ViewHolder> extends SimpleStatefulAdaptor<T> {
        abstract public boolean isLoaded();
    }
}
