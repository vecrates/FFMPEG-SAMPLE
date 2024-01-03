package cn.vecrates.ffmpeg.render;

import android.opengl.EGLContext;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Size;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;

import cn.vecrates.ffmpeg.BuildConfig;
import cn.vecrates.ffmpeg.ffmpeg.FFDecodeListener;
import cn.vecrates.ffmpeg.ffmpeg.FFPlayer;
import cn.vecrates.ffmpeg.ffmpeg.PCMPlayer;
import cn.vecrates.ffmpeg.render.common.GLHandler;

public class VideoDrawer {

    private static final String TAG = VideoDrawer.class.getSimpleName();

    /**
     * surfaceView size
     */
    private Size surfaceSize;
    /**
     * viewport size
     * 通常等于surfaceSize
     */
    private Size viewportSize;
    /**
     * 渲染size
     * 如果未设置裁剪，将等于视频帧size
     */
    private Size renderSize;

    private GLHandler glHandler;
    private Handler manageHandler;

    private FFPlayer ffPlayer;
    private PCMPlayer pcmPlayer;

    private AbsVideoDrawerProxy drawerProxy;
    private boolean releasableDrawerProxy;

    private DrawerListener drawerListener;

    public VideoDrawer() {
        initGLHandler();
        initManageHandler();
    }

    private void initGLHandler() {
        glHandler = new GLHandler();
        glHandler.setGLHandlerListener(glHandlerListener);
        glHandler.createGLContext();
    }

    private void initManageHandler() {
        HandlerThread thread = new HandlerThread("manageThread");
        thread.start();
        manageHandler = new Handler(thread.getLooper());
    }

    public void setSurfaceView(@NonNull SurfaceView surfaceView) {
        surfaceView.getHolder().addCallback(surfaceListener);
    }

    public void setDrawerProxy(@NonNull AbsVideoDrawerProxy drawerProxy, boolean releasable) {
        this.drawerProxy = drawerProxy;
        this.releasableDrawerProxy = releasable;
        this.drawerProxy.setGLThreadProxy(glThreadProxy);
    }

    public void setDrawerListener(DrawerListener drawerListener) {
        this.drawerListener = drawerListener;
    }

    public void prepare(String path) {
        managePost(() -> {
            releaseFFDecoder();
            ffPlayer = new FFPlayer();
            ffPlayer.prepare(path);
            ffPlayer.setDecodeListener(ffDecodeListener);
            initAudioPlayer();
            initDrawer();
            if (drawerListener != null) {
                drawerListener.onPrepared();
            }
        });
    }

    private void initAudioPlayer() {
        try {
            pcmPlayer = new PCMPlayer();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private void releaseFFDecoder() {
        if (ffPlayer != null) {
            ffPlayer.release();
            ffPlayer = null;
        }
    }

    private void initDrawer() {
        glPost(() -> {
            if (drawerProxy != null) {
                drawerProxy.init();
            }
            int[] size = ffPlayer.getVideoSize();
            renderSize = new Size(size[0], size[1]);
            notifySizeChanged();
        });
    }

    private final SurfaceHolder.Callback surfaceListener = new SurfaceHolder.Callback() {
        @Override
        public void surfaceCreated(@NonNull SurfaceHolder holder) {
            Log.d(TAG, "surfaceCreated: ");
            if (glHandler != null) {
                glHandler.createGLSurface(holder.getSurface());
            }
        }

        @Override
        public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
            Log.d(TAG, "surfaceChanged: " + width + "x" + height);
            surfaceSize = new Size(width, height);
            viewportSize = new Size(width, height);
            glPost(() -> notifySizeChanged());
        }

        @Override
        public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
            Log.d(TAG, "surfaceDestroyed: ");
            if (glHandler != null) {
                glHandler.releaseGLSurface();
            }
        }
    };

    private final IGLThreadProxy glThreadProxy = new IGLThreadProxy() {

        @Override
        public void post(@NonNull Runnable r) {
            if (glHandler != null) {
                glHandler.runOnGLThread(r);
            }
        }

        @Override
        public void postDraw(@NonNull Runnable r) {
            if (glHandler != null) {
                glHandler.runOnGLThread(() -> {
                    r.run();
                    glHandler.requestRenderSync();
                });
            }
        }

        @Override
        public EGLContext getGLContext() {
            return glHandler != null && glHandler.getGLCore() != null ?
                    glHandler.getGLCore().getEGLContext() : null;
        }

        @NonNull

        @Override
        public Size getSurfaceSize() {
            return surfaceSize != null ? surfaceSize : new Size(0, 0);
        }
    };

    private final GLHandler.GLHandlerListener glHandlerListener = new GLHandler.GLHandlerListener() {
        @Override
        public void onGLContextCreated() {
            Log.d(TAG, "onGLContextCreated: ");
        }

        @Override
        public void onGLSurfaceCreated() {
            Log.d(TAG, "onGLSurfaceCreated: ");
        }

        @Override
        public void onGLSurfaceDestroyed() {
            Log.d(TAG, "onGLSurfaceDestroyed: ");
        }

        @Override
        public void onGLContextShutdown() {
            Log.d(TAG, "onGLContextShutdown: ");
        }

        @Override
        public void onDrawFrame() {
            try {
                if (drawerProxy != null) {
                    drawerProxy.draw();
                }
            } catch (Exception e) {
                Log.e(TAG, "onDrawFrame: " + e.getMessage());
                if (BuildConfig.DEBUG) {
                    throw e;
                }
            }
        }

    };

    private final FFDecodeListener ffDecodeListener = new FFDecodeListener() {
        @Override
        public void onVideoFrameAvailable(byte[] y, byte[] u, byte[] v, int width, int height) {
            if (glHandler == null) {
                return;
            }
            glHandler.clearDrawMessages();
            glHandler.postDrawMessage(() -> {
                if (drawerProxy != null) {
                    drawerProxy.updateYUV(y, u, v, width, height);
                }
                if (glHandler != null) {
                    glHandler.requestRenderSync();
                }
            });
        }

        @Override
        public void onAudioFrameAvailable(byte[] pcmArray) {
            if (pcmPlayer != null) {
                pcmPlayer.writePcm(pcmArray);
            }
        }
    };

    public void managePost(@NonNull Runnable r) {
        if (manageHandler != null) {
            manageHandler.post(r);
        }
    }

    public void glPost(@NonNull Runnable r) {
        if (glHandler != null) {
            glHandler.runOnGLThread(r);
        }
    }

    /**
     * call on gl
     */
    private void notifySizeChanged() {
        if (viewportSize == null || renderSize == null) {
            return;
        }
        if (drawerProxy != null) {
            drawerProxy.onSizeChanged(viewportSize, renderSize);
        }
    }

    public void release() {
        glPost(() -> {
            if (ffPlayer != null) {
                ffPlayer.release();
                ffPlayer = null;
            }
            if (pcmPlayer != null) {
                pcmPlayer.release();
                pcmPlayer = null;
            }
            if (drawerProxy != null && releasableDrawerProxy) {
                drawerProxy.release();
                drawerProxy = null;
            }
            if (glHandler != null) {
                glHandler.shutdownSync();
                glHandler = null;
            }
        });
    }

    //region control

    @NonNull
    public Size getVideoSize() {
        int[] size = ffPlayer.getVideoSize();
        return new Size(size[0], size[1]);
    }

    public void start() {
        managePost(() -> {
            if (ffPlayer != null) {
                ffPlayer.start();
            }
            if (pcmPlayer != null) {
                pcmPlayer.start();
            }
        });
    }

    public void stop() {
        managePost(() -> {
            if (ffPlayer != null) {
                ffPlayer.stop();
            }
            if (pcmPlayer != null) {
                pcmPlayer.stop();
            }
        });
    }

    //endregion

}