package cn.vecrates.ffmpeg.manager;

import cn.vecrates.ffmpeg.App;
import cn.vecrates.ffmpeg.util.FileUtil;

public class AudioManager {

    private AudioManager() {

    }

    public static void init() {
        FileUtil.copyAssets(App.context, "audio", getAudioDir());
    }

    public static String getAudioDir() {
        return App.context.getFilesDir().getPath();
    }

}
