package cn.vecrates.ffmpeg;

import android.Manifest;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import cn.vecrates.ffmpeg.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        init();

        requestPermissions();
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

        binding.tvCmd.setOnClickListener(v -> {
            Intent intent = new Intent(this, CmdActivity.class);
            startActivity(intent);
        });

        binding.tvExtractFrames.setOnClickListener(v -> {
            Intent intent = new Intent(this, ExtractFramesActivity.class);
            startActivity(intent);
        });
    }

    private void requestPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            requestPermissions(new String[]{Manifest.permission.READ_MEDIA_IMAGES, Manifest.permission.READ_MEDIA_VIDEO}, 0);
        } else {
            requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 0);
        }
    }

}