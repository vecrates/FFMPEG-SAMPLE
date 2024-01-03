package cn.vecrates.ffmpeg.manager;

import cn.vecrates.ffmpeg.App;
import cn.vecrates.ffmpeg.util.FileUtil;

public class MediaManager {

    private MediaManager() {

    }

    public static void init() {
        FileUtil.copyAssets(App.context, "audio", getMediaDir());
        FileUtil.copyAssets(App.context, "video", getMediaDir());
    }

    public static String getMediaDir() {
        return App.context.getFilesDir().getPath();
    }

}
