package cn.vecrates.ffmpeg;

import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import cn.vecrates.ffmpeg.ffmpeg.AudioMixControl;

public class AudioMixerActivity extends AppCompatActivity {

    private AudioMixControl audioMixControl;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio_mixer);

        audioMixControl = new AudioMixControl();
        audioMixControl.init();

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (audioMixControl != null) {
            audioMixControl.release();
            audioMixControl = null;
        }
    }

}