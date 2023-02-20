package cn.vecrates.ffmpeg;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;

import com.zhihu.matisse.Matisse;
import com.zhihu.matisse.MimeType;
import com.zhihu.matisse.engine.impl.GlideEngine;

import java.util.List;

import cn.vecrates.ffmpeg.databinding.ActivityMainBinding;
import cn.vecrates.ffmpeg.ffplayer.FFPlayer;
import cn.vecrates.ffmpeg.util.ThreadHelper;
import cn.vecrates.ffmpeg.util.ToastUtil;
import cn.vecrates.ffmpeg.util.UriUtil;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    private final static int ALBUM_REQUEST_CODE = 1;

    private ActivityMainBinding binding;

    private FFPlayer ffPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        initView();

        initPlayer();

    }

    private void initView() {
        binding.tvAlbum.setOnClickListener(this::onClickAlbum);
        binding.tvPlay.setOnClickListener(this::onClickPlay);
    }

    private void initPlayer() {
        ffPlayer = new FFPlayer();
    }

    private void updatePlayer(String path) {
        ffPlayer.reset();
        ffPlayer.prepare(path);
        updateSurface(path);
    }

    private void updateSurface(String path) {
        ThreadHelper.runBackground(() -> {
            MediaMetadataRetriever retriever = null;
            try {
                retriever = new MediaMetadataRetriever();
                retriever.setDataSource(path);
                int width = Integer.parseInt(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH));
                int height = Integer.parseInt(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT));
                ThreadHelper.runOnUIThread(() -> {
                    binding.svVideo.getHolder().addCallback(surfaceCallback);
                    int usableHeight = binding.tvAlbum.getTop();
                    int usableWidth = binding.clRoot.getWidth();
                    float ratioUsable = (float) usableWidth / usableHeight;
                    float ratioVideo = (float) width / height;
                    int finalWidth = usableWidth;
                    int finalHeight = (int) (finalWidth / ratioVideo);
                    if (ratioUsable > ratioVideo) {
                        finalHeight = usableHeight;
                        finalWidth = (int) (finalHeight * ratioVideo);
                    }
                    ConstraintLayout.LayoutParams params = (ConstraintLayout.LayoutParams) binding.svVideo.getLayoutParams();
                    params.width = finalWidth;
                    params.height = finalHeight;
                    binding.svVideo.setLayoutParams(params);
                });
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                if (retriever != null) {
                    retriever.release();
                }
            }
        });
    }

    private final SurfaceHolder.Callback surfaceCallback = new SurfaceHolder.Callback() {
        @Override
        public void surfaceCreated(@NonNull SurfaceHolder holder) {

        }

        @Override
        public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
            binding.svVideo.getHolder().removeCallback(surfaceCallback);
            ToastUtil.show("updated surface: width=" + width + " height=" + height);
        }

        @Override
        public void surfaceDestroyed(@NonNull SurfaceHolder holder) {

        }
    };

    private void onClickPlay(View view) {
        ffPlayer.start();
    }

    private void onClickAlbum(View v) {
        Matisse.from(MainActivity.this)
                .choose(MimeType.ofVideo())
                .countable(true)
                .maxSelectable(1)
                .showSingleMediaType(true)
                .autoHideToolbarOnSingleTap(true)
                .restrictOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED)
                .thumbnailScale(0.85f)
                .imageEngine(new GlideEngine())
                .showPreview(false)
                .forResult(ALBUM_REQUEST_CODE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != ALBUM_REQUEST_CODE || resultCode != RESULT_OK) {
            return;
        }
        List<Uri> uris = Matisse.obtainResult(data);
        if (uris.isEmpty()) {
            return;
        }
        Uri uri = uris.get(0);
        String path = UriUtil.queryUriPath(this, uri);
        Log.e(TAG, "onActivityResult: uri=" + uri.toString() + " path=" + path);
        updatePlayer(path);
    }

}