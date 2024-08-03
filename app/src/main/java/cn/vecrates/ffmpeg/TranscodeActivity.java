package cn.vecrates.ffmpeg;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;

import androidx.appcompat.app.AppCompatActivity;

import java.io.File;

import cn.vecrates.ffmpeg.ffmpeg.FFEncoder;
import cn.vecrates.ffmpeg.manager.MediaManager;

public class TranscodeActivity extends AppCompatActivity {

    private Handler handler;
    private FFEncoder ffEncoder;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_transcode);

        HandlerThread thread = new HandlerThread("transcode");
        thread.start();
        handler = new Handler(thread.getLooper());

        handler.post((() -> {
            String src = MediaManager.getMediaDir() + "/test.mp4";
            String dest = MediaManager.getMediaDir() + "/test_transcode.mp4";
            File file = new File(dest);
            if (file.exists()) {
                file.delete();
            }
            ffEncoder = new FFEncoder();
            if (ffEncoder.init(src, dest)) {
                ffEncoder.start();
            }
        }));

    }

    @Override
    protected void onPause() {
        super.onPause();
        if (isFinishing()) {
            release();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        release();
    }

    private void release() {
        if (handler == null) {
            return;
        }
        handler.post(() -> {
            if (ffEncoder != null) {
                ffEncoder.stop();
                ffEncoder.destroy();
                ffEncoder = null;
            }
        });
        handler.getLooper().quitSafely();
        handler = null;
    }

}