package cn.vecrates.ffmpeg.ffmpeg;

import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import cn.vecrates.ffmpeg.manager.MediaManager;

public class AudioMixControl {

    private AudioMixer audioMixer;
    private Handler handler;

    private PCMPlayer pcmPlayer;

    private volatile boolean play;

    public AudioMixControl() {
        HandlerThread handlerThread = new HandlerThread("AudioMixControlThread");
        handlerThread.start();
        handler = new Handler(handlerThread.getLooper());

        try {
            pcmPlayer = new PCMPlayer();
            pcmPlayer.start();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public void init() {
        handler.post(() -> {
            audioMixer = new AudioMixer();
            audioMixer.addAudio(MediaManager.getMediaDir() + "/1.mp3");
            audioMixer.addAudio(MediaManager.getMediaDir() + "/2.mp3");
//            audioMixer.addAudio(AudioManager.getAudioDir() + "/4.wav");
//            audioMixer.addAudio(AudioManager.getAudioDir() + "/4.wav");
//            audioMixer.addAudio(AudioManager.getAudioDir() + "/4.wav");
//            audioMixer.addAudio(AudioManager.getAudioDir() + "/4.wav");
//            audioMixer.addAudio(AudioManager.getAudioDir() + "/4.wav");
            if (!audioMixer.init()) {
                return;
            }
            play = true;
            while (play) {
                byte[] bytes = audioMixer.readFrame();
                pcmPlayer.writePcm(bytes);
                //Log.e("读取PCM", bytes.length + "-->"  + Arrays.toString(bytes));
            }
            if (audioMixer != null) {
                Log.e("===", "release audio mixer ");
                audioMixer.destroy();
                audioMixer = null;
            }
        });
    }

    public void release() {
        play = false;
        if (handler != null) {
            handler.getLooper().quitSafely();
            handler = null;
        }
    }

}
