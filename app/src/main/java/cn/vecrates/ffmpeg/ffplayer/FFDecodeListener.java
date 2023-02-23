package cn.vecrates.ffmpeg.ffplayer;

public interface FFDecodeListener {

    void onVideoFrameAvailable(byte[] y, byte[] u, byte[] v);

    void onAudioFrameAvailable(byte[] pcmArray);

}
