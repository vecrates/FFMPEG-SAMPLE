package cn.vecrates.ffmpeg.render;

import android.opengl.EGLContext;
import android.util.Size;

import androidx.annotation.NonNull;

import java.util.ArrayList;
import java.util.List;

import cn.vecrates.ffmpeg.render.common.GLFrameBuffer;
import cn.vecrates.ffmpeg.render.pass.BasePass;

/**
 * 渲染流程的基类
 * 定义了渲染的基本框架
 */
public abstract class AbsDrawerProxy {

    private static final String TAG = AbsDrawerProxy.class.getSimpleName();

    protected boolean initialized;

    protected int renderWidth;
    protected int renderHeight;
    protected int viewportWidth;
    protected int viewportHeight;

    private GLFrameBuffer[] frameBuffers;
    private int frameBufferIndex = 0;

    protected final List<BasePass> passes = new ArrayList<>(5);

    private IGLThreadProxy glThreadProxy;

    public AbsDrawerProxy() {

    }

    public final void init() {
        if (initialized) {
            return;
        }

        frameBuffers = new GLFrameBuffer[3];
        for (int i = 0; i < frameBuffers.length; i++) {
            frameBuffers[i] = new GLFrameBuffer();
        }

        initPasses();

        initialized = true;
    }

    protected abstract void initPasses();

    public boolean isInitialized() {
        return initialized;
    }

    //region pass context

    protected final PassContext passContext = new PassContext() {

        @Override
        public void postDraw(Runnable runnable) {
            if (glThreadProxy != null) {
                glThreadProxy.postDraw(runnable);
            }
        }

        @Override
        public void post(Runnable runnable) {
            if (glThreadProxy != null) {
                glThreadProxy.post(runnable);
            }
        }

        @Override
        public EGLContext getGLContext() {
            return glThreadProxy != null ? glThreadProxy.getGLContext() : null;
        }

        @Override
        public Size getSurfaceSize() {
            return glThreadProxy != null ? glThreadProxy.getSurfaceSize() : new Size(0, 0);
        }

        @Override
        public GLFrameBuffer nextFrameBuffer() {
            return frameBuffers[frameBufferIndex++ % frameBuffers.length];
        }

    };

    //endregion

    public void setGLThreadProxy(IGLThreadProxy glThreadProxy) {
        this.glThreadProxy = glThreadProxy;
    }

    public void onSizeChangedAsync(@NonNull Size viewportSize, @NonNull Size renderSize) {
        passContext.post(() -> onSizeChanged(viewportSize, renderSize));
    }

    public void onSizeChanged(@NonNull Size viewportSize, @NonNull Size renderSize) {
        this.renderWidth = renderSize.getWidth();
        this.renderHeight = renderSize.getHeight();
        this.viewportWidth = viewportSize.getWidth();
        this.viewportHeight = viewportSize.getHeight();
        for (BasePass pass : passes) {
            pass.onSizeChanged(renderWidth, renderHeight, viewportWidth, viewportHeight);
        }
    }

    public void draw() {
        int width = renderWidth;
        int height = renderHeight;

        if (!initialized || width <= 0 || height <= 0) {
            return;
        }

        int textureId = -1;
        for (BasePass pass : passes) {
            textureId = pass.draw(textureId, width, height);
        }

    }

    protected final void addPass(BasePass pass) {
        passes.add(pass);
    }

    //region access

    public abstract BasePass getFormatPass();

    public abstract BasePass getDisplayPass();

    public int getRenderWidth() {
        return renderWidth;
    }

    public int getRenderHeight() {
        return renderHeight;
    }

    public Size getViewportSize() {
        return new Size(viewportWidth, viewportHeight);
    }

    public Size getRenderSize() {
        return new Size(renderWidth, renderHeight);
    }

    /**
     * call on GL
     */
    public void release() {
        initialized = false;
        if (frameBuffers != null) {
            for (GLFrameBuffer frameBuffer : frameBuffers) {
                frameBuffer.destroyFrameBuffer();
            }
            frameBuffers = null;
        }
        for (BasePass pass : passes) {
            pass.release();
        }
    }

    //endregion

}
