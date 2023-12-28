package cn.vecrates.ffmpeg;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.util.Size;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;

import com.zhihu.matisse.Matisse;
import com.zhihu.matisse.MimeType;
import com.zhihu.matisse.engine.impl.GlideEngine;

import java.util.List;

import cn.vecrates.ffmpeg.databinding.ActivityMainBinding;
import cn.vecrates.ffmpeg.ffmpeg.AudioMixControl;
import cn.vecrates.ffmpeg.render.DrawerListener;
import cn.vecrates.ffmpeg.render.VideoDrawer;
import cn.vecrates.ffmpeg.render.VideoDrawerProxy;
import cn.vecrates.ffmpeg.util.ScreenUtil;
import cn.vecrates.ffmpeg.util.ThreadHelper;
import cn.vecrates.ffmpeg.util.UriUtil;

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

    }


}