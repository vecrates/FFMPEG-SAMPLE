package cn.vecrates.ffmpeg.render;

import android.view.Surface;

public class FFRender {

    static {
        System.loadLibrary("ffplayer");
    }

    public void prepare(String path, Surface surface) {
        playVideoByOpenGL(path, surface);
    }

    private native void playVideoByOpenGL(String path, Surface surface);


}
