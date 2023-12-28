package cn.vecrates.ffmpeg.ffmpeg;

public interface FFDecodeListener {

    void onVideoFrameAvailable(byte[] y, byte[] u, byte[] v, int width, int height);

    void onAudioFrameAvailable(byte[] pcmArray);

}
