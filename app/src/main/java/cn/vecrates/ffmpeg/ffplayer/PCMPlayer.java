package cn.vecrates.ffmpeg.ffplayer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

public class PCMPlayer {

    private static final String TAG = "PCMPlayer";

    protected AudioTrack audioTrack;
    protected float curVolume;

    public PCMPlayer() throws Exception {
        int bufSize = AudioTrack.getMinBufferSize(44100, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, 44100, AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT, bufSize, AudioTrack.MODE_STREAM);
        audioTrack.setStereoVolume(1, 1);
        curVolume = 1;
    }

    public boolean isInited() {
        return audioTrack != null && audioTrack.getState() == AudioTrack.STATE_INITIALIZED;
    }

    public void start() {
        if (audioTrack == null || audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            return;
        }
        try {
            audioTrack.flush();
            audioTrack.play();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void stop() {
        if (audioTrack == null || audioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING) {
            return;
        }
        try {
            audioTrack.stop();
            audioTrack.flush();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void writePcm(byte[] pcms) {
        try {
            if (audioTrack != null && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
                audioTrack.write(pcms, 0, pcms.length);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void setVolume(float volume) {
        if (audioTrack == null) {
            return;
        }
        volume = Math.max(0, volume);
        volume = Math.min(1, volume);
        this.curVolume = volume;
        audioTrack.setStereoVolume(volume, volume);
    }

    public float getCurrentVolume() {
        return curVolume;
    }

    public void release() {
        if (audioTrack != null) {
            try {
                audioTrack.stop();
            } catch (Exception e) {
                e.printStackTrace();
            }
            audioTrack.release();
            audioTrack = null;
        }
    }

}
