package cn.vecrates.ffmpeg.coord;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.coordinatorlayout.widget.CoordinatorLayout;

import com.google.android.material.appbar.AppBarLayout;

import cn.vecrates.ffmpeg.R;
import cn.vecrates.ffmpeg.TRadiusLinearLayout;

public class TabBarBackgroundBehavior extends CoordinatorLayout.Behavior<View> {

    public TabBarBackgroundBehavior() {
        super();
    }

    public TabBarBackgroundBehavior(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean layoutDependsOn(@NonNull CoordinatorLayout parent,
                                   @NonNull View child,
                                   @NonNull View dependency) {
        Log.e("====", "layoutDependsOn: " + child + " " + dependency.getId() + " " + dependency);
        return dependency instanceof AppBarLayout;
    }

    @Override
    public boolean onDependentViewChanged(@NonNull CoordinatorLayout parent,
                                          @NonNull View child, @NonNull View dependency) {
        View appBar = parent.findViewById(R.id.layout_app_bar);
        View tabBar = parent.findViewById(R.id.ll_tab_bar);
        View tabTop = parent.findViewById(R.id.view_tab_top);

        Log.e("====", "onDependentViewChanged: "
                + appBar.getTop()
                + " " + appBar.getScrollY());

        int maxOffset = (tabTop.getHeight() - tabTop.getMinimumHeight());
        int offset = Math.abs(appBar.getTop());
        float factor = 1.f - offset / (maxOffset * 1.f);
        Log.e("====", "onDependentViewChanged: " + factor);
        ((TRadiusLinearLayout) tabBar).setRadiusFactor(factor);
        return true;
    }
}
