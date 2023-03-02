package cn.vecrates.ffmpeg.util;

import android.widget.Toast;

import cn.vecrates.ffmpeg.App;


public class ToastUtil {

    public static void show(String text) {
        ThreadHelper.runOnUIThread(() -> {
            Toast.makeText(App.context, text, Toast.LENGTH_LONG).show();
        });
    }


}
