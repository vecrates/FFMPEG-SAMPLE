package cn.vecrates.ffmpeg.util;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.util.DisplayMetrics;
import android.view.WindowManager;

import cn.vecrates.ffmpeg.App;

public class ScreenUtil {

    @SuppressLint("StaticFieldLeak")
    private static final Context context = App.context;

    private static int screenHeight;
    private static int screenWidth;

    public static int dp2px(float dpValue) {
        final float scale = context.getResources().getDisplayMetrics().density;
        return (int) (dpValue * scale + 0.5f);
    }

    public static int px2dp(float pxValue) {
        final float scale = context.getResources().getDisplayMetrics().density;
        return (int) (pxValue / scale + 0.5f);
    }

    public static int px2sp(float pxValue) {
        final float fontScale = context.getResources().getDisplayMetrics().scaledDensity;
        return (int) (pxValue / fontScale + 0.5f);
    }

    public static int sp2px(float spValue) {
        final float fontScale = context.getResources().getDisplayMetrics().scaledDensity;
        return (int) (spValue * fontScale + 0.5f);
    }

    public static int screenHeight() {
        if (screenHeight == 0) {
            screenHeight = getDisplayMetrics().heightPixels;
        }
        return screenHeight;
    }

    public static int screenWidth() {
        if (screenWidth == 0) {
            screenWidth = getDisplayMetrics().widthPixels;
        }
        return screenWidth;
    }

    public static DisplayMetrics getDisplayMetrics() {
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics outMetrics = new DisplayMetrics();
        if (wm != null) wm.getDefaultDisplay().getMetrics(outMetrics);
        return outMetrics;
    }

    public static void setKeepScreenOn(Activity activity, boolean keepScreenOn) {
        if (activity == null || activity.getWindow() == null) {
            return;
        }
        if (keepScreenOn) {
            activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        } else {
            activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
    }

}
