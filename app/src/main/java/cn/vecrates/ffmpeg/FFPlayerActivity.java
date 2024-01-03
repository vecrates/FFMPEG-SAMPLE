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

import cn.vecrates.ffmpeg.databinding.ActivityFfplayerBinding;
import cn.vecrates.ffmpeg.render.DrawerListener;
import cn.vecrates.ffmpeg.render.VideoDrawer;
import cn.vecrates.ffmpeg.render.VideoDrawerProxy;
import cn.vecrates.ffmpeg.util.ScreenUtil;
import cn.vecrates.ffmpeg.util.ThreadHelper;
import cn.vecrates.ffmpeg.util.UriUtil;

public class FFPlayerActivity extends AppCompatActivity {

    private static final String TAG = "FFPlayerActivity";

    private final static int ALBUM_REQUEST_CODE = 1;

    private ActivityFfplayerBinding binding;

    private VideoDrawer videoDrawer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityFfplayerBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        initView();

        ScreenUtil.setKeepScreenOn(this, true);

    }

    private void initView() {
        binding.tvAlbum.setOnClickListener(this::onClickAlbum);
        binding.tvPlay.setOnClickListener(this::onClickPlay);
    }

    private void updatePlayer(String path) {
        if (videoDrawer != null) {
            videoDrawer.release();
            videoDrawer = null;
        }
        videoDrawer = new VideoDrawer();
        videoDrawer.setSurfaceView(binding.svVideo);
        videoDrawer.setDrawerProxy(new VideoDrawerProxy(), true);
        videoDrawer.prepare(path);
        videoDrawer.setDrawerListener(drawerListener);
    }

    private final DrawerListener drawerListener = new DrawerListener() {
        @Override
        public void onPrepared() {
            ThreadHelper.runOnUIThread(() -> {
                if (isDestroyed() || isFinishing()) {
                    return;
                }
                updateSurfaceSize();
            });
        }
    };

    private void updateSurfaceSize() {
        Size size = videoDrawer.getVideoSize();
        int usableHeight = binding.tvAlbum.getTop() - ScreenUtil.dp2px(40);
        int usableWidth = binding.clRoot.getWidth();
        float ratioUsable = (float) usableWidth / usableHeight;
        float ratioVideo = (float) size.getWidth() / size.getHeight();
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

    }

    private void onClickPlay(View view) {
        videoDrawer.start();
    }

    private void onClickAlbum(View v) {
        Matisse.from(this)
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
        if (videoDrawer != null) {
            videoDrawer.stop();
            videoDrawer.release();
            videoDrawer = null;
        }
    }


}