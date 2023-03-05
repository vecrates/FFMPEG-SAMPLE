package cn.vecrates.ffmpeg.ffplayer;

public interface FFDecodeListener {

    void onVideoFrameAvailable(byte[] y, byte[] u, byte[] v, int width, int height);

    void onAudioFrameAvailable(byte[] pcmArray);

}
