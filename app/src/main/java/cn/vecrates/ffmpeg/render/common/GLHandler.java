package cn.vecrates.ffmpeg.render.common;

import android.graphics.SurfaceTexture;
import android.opengl.EGLSurface;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.util.Log;
import android.view.Surface;

public class GLHandler implements Handler.Callback {

    private static final String TAG = GLHandler.class.getSimpleName();

    public static final int GL_CREATE_CONTEXT = 0;
    public static final int GL_RELEASE_GLSURFACE = 1;
    public static final int GL_SHUTDOWN = 2;
    public static final int GL_RECREATE_GLSURFACE = 3;
    public static final int GL_DRAW = 4;

    private HandlerThread glThread;
    private Handler handler;
    private GLCore eglCore;
    private GLSurface previewGLSurface;
    private EGLSurface offscreenSurface;
    private Surface surface;

    private boolean useGL3 = false;

    private GLHandlerListener listener;

    private volatile boolean destroyed = true;

    public GLHandler() {
        glThread = new HandlerThread("GLHandlerThread");
        glThread.start();
        handler = new Handler(glThread.getLooper(), this);
    }

    public void setGLHandlerListener(GLHandlerListener listener) {
        this.listener = listener;
    }

    public boolean isGLThread(Thread thread) {
        return thread == glThread;
    }

    public void createGLContext() {
        if (handler == null) return;
        handler.sendMessage(handler.obtainMessage(GL_CREATE_CONTEXT));
    }

    public void createGLSurface(Surface surface) {
        this.surface = surface;
        if (handler == null) return;
        handler.sendMessage(handler.obtainMessage(GL_RECREATE_GLSURFACE));
    }

    /**
     * call on gl
     *
     * @param surface
     */
    public void createGLSurfaceSync(Surface surface) {
        this.surface = surface;
        if (handler == null) return;
        recreateGLSurfaceInner();
    }

    public void releaseGLSurface() {
        if (handler == null) return;
        handler.sendMessage(handler.obtainMessage(GL_RELEASE_GLSURFACE));
    }

    public void shutdown() {
        if (handler == null) return;
        handler.sendMessage(handler.obtainMessage(GL_SHUTDOWN));
    }

    /**
     * call on gl
     */
    public void shutdownSync() {
        if (handler == null) {
            return;
        }
        if (!isGLThread(Thread.currentThread())) {
            return;
        }
        doShutdown();
    }

    public void setUseGL3() {
        this.useGL3 = true;
    }

    public boolean supportGL3() {
        return eglCore != null && eglCore.getGlVersion() >= 3;
    }

    public GLCore getGLCore() {
        return eglCore;
    }

    public GLSurface getPreviewGLSurface() {
        return previewGLSurface;
    }

    public boolean isGlSurfaceNull() {
        return previewGLSurface == null;
    }

    public boolean isGLContextDestroyed() {
        return destroyed;
    }

    public void runOnGLThread(Runnable runnable) {
        if (handler == null) return;
        handler.post(runnable);
    }

    public void runOnGLThread(Runnable runnable, long delay) {
        if (handler == null) return;
        handler.postDelayed(runnable, delay);
    }

    public void requestRender(SurfaceTexture surfaceTexture) {
        if (handler == null) return;
        handler.sendMessage(handler.obtainMessage(GL_DRAW, surfaceTexture));
    }

    public void postDrawMessage(Runnable runnable) {
        if (handler == null) return;
        Message message = Message.obtain(handler, runnable);
        message.what = GL_DRAW;
        handler.sendMessage(message);
    }

    public void requestRenderSync() {
        if (destroyed) return;
        draw();
    }

    public void clearMessages() {
        if (handler == null) return;
        handler.removeCallbacksAndMessages(null);
    }

    public void clearDrawMessages() {
        if (handler == null) return;
        handler.removeMessages(GL_DRAW);
    }

    public Handler getHandler() {
        return handler;
    }

    @Override
    public boolean handleMessage(Message msg) {
        switch (msg.what) {
            case GL_CREATE_CONTEXT:
                createContextInner();
                break;
            case GL_RELEASE_GLSURFACE:
                releaseGLSurfaceInner(true);
                break;
            case GL_SHUTDOWN:
                doShutdown();
                break;
            case GL_RECREATE_GLSURFACE:
                recreateGLSurfaceInner();
                break;
            case GL_DRAW:
                draw();
                break;
            default:
                break;
        }
        return false;
    }

    private void createContextInner() {
        if (eglCore == null) {
            try {
                eglCore = new GLCore(null, useGL3 ? GLCore.FLAG_TRY_GLES3 : GLCore.FLAG_RECORDABLE);
            } catch (Exception e) {
                Log.e(TAG, "******\ndoCreateContext: create glCore failed\n******");
                e.printStackTrace();
                return;
            }
        }

        try {
            offscreenSurface = eglCore.createOffscreenSurface(2, 2);
            eglCore.makeCurrent(offscreenSurface);
        } catch (Exception e) {
            Log.e(TAG, "*******\ndoCreateContext: create EGLSurface failed\n********");
            e.printStackTrace();
            return;
        }

        destroyed = false;
        if (listener != null) {
            listener.onGLContextCreated();
        }
    }

    private void recreateGLSurfaceInner() {
        releaseGLSurfaceInner(false);
        try {
            previewGLSurface = new GLSurface(eglCore, surface, false);
            previewGLSurface.makeCurrent();
        } catch (Exception e) {
            e.printStackTrace();
            Log.e(TAG, "******\ndoRecreateGLSurface: create EGLSurface failed\n******");
            releaseGLSurfaceInner(false);
            return;
        }

        /*draw(null);*/

        if (listener != null) {
            listener.onGLSurfaceCreated();
        }
    }

    private void draw() {
        if (previewGLSurface == null || listener == null) {
            return;
        }

        listener.onDrawFrame();
        previewGLSurface.swapBuffers();
    }

    /**
     * call on gl
     */
    public void makeCurrent() {
        if (previewGLSurface != null) {
            previewGLSurface.makeCurrent();
        }
    }

    private void releaseGLSurfaceInner(boolean callback) {
        //某些机型退到桌面再回到播放页面会无法绘制，给一个离屏的surface解决
        if (eglCore != null && offscreenSurface != null) {
            eglCore.makeCurrent(offscreenSurface);
        }
        if (previewGLSurface != null) {
            previewGLSurface.release();
            previewGLSurface = null;
        }
        if (callback && listener != null) {
            listener.onGLSurfaceDestroyed();
        }
    }

    private void doShutdown() {
        destroyed = true;

        eglCore.makeNothingCurrent();
        if (previewGLSurface != null) {
            previewGLSurface.release();
            previewGLSurface = null;
        }

        if (offscreenSurface != null && eglCore != null) {
            eglCore.releaseSurface(offscreenSurface);
            offscreenSurface = null;
        }

        if (eglCore != null) {
            eglCore.release();
            eglCore = null;
        }

        if (glThread != null) {
            glThread.quit();
            glThread = null;
        }

        handler = null;

        if (listener != null) {
            listener.onGLContextShutdown();
        }

    }

    public interface GLHandlerListener {

        void onGLContextCreated();

        void onGLSurfaceCreated();

        void onGLSurfaceDestroyed();

        void onGLContextShutdown();

        void onDrawFrame();

    }

}
