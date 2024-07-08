package cn.vecrates.ffmpeg;

import android.content.Intent;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import cn.vecrates.ffmpeg.databinding.ActivityMainBinding;
import cn.vecrates.ffmpeg.render.VideoDrawer;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    private ActivityMainBinding binding;

    private VideoDrawer videoDrawer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        init();

    }

    private void init() {
        binding.tvFfplayer.setOnClickListener(v -> {
            Intent intent = new Intent(this, FFPlayerActivity.class);
            startActivity(intent);
        });

        binding.tvAudioMixer.setOnClickListener(v -> {
            Intent intent = new Intent(this, AudioMixerActivity.class);
            startActivity(intent);
        });

        binding.tvTranscode.setOnClickListener(v -> {
            Intent intent = new Intent(this, TranscodeActivity.class);
            startActivity(intent);
        });

    }

}