package cn.vecrates.ffmpeg;

import android.app.Application;
import android.content.Context;

import cn.vecrates.ffmpeg.manager.AudioManager;
import cn.vecrates.ffmpeg.util.ThreadHelper;

public class App extends Application {

    public static Context context;

    @Override
    public void onCreate() {
        super.onCreate();
        context = getApplicationContext();
        initBackground();
    }

    private void initBackground() {
        ThreadHelper.runBackground(() -> {
            AudioManager.init();
        });
    }

    static {
        System.loadLibrary("ffmpeg");
    }

}
