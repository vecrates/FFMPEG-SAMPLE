package cn.vecrates.ffmpeg;

import android.app.Application;
import android.content.Context;

import cn.vecrates.ffmpeg.manager.MediaManager;
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
            MediaManager.init();
        });
    }

    static {
        System.loadLibrary("x264");
        System.loadLibrary("ffmpeg");
    }

}
