package cn.vecrates.ffmpeg.render.pass;

import android.graphics.RectF;
import android.util.Size;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;

import cn.vecrates.ffmpeg.render.PassContext;
import cn.vecrates.ffmpeg.render.util.MathUtil;


public abstract class BasePass {

    protected PassContext passContext;

    /**
     * 渲染帧尺寸（frameBuffer）
     */
    protected int renderWidth;
    protected int renderHeight;

    /**
     * 显示帧尺寸
     */
    protected int displayWidth;
    protected int displayHeight;

    /**
     * 绘制视口
     */
    protected int viewportWidth;
    protected int viewportHeight;

    public BasePass(@NonNull PassContext passContext) {
        this.passContext = passContext;
    }

    @CallSuper
    protected void init() {

    }

    /**
     * @param renderWidth    渲染大小
     * @param renderHeight
     * @param viewportWidth  视口大小
     * @param viewportHeight
     */
    @CallSuper
    public void onSizeChanged(int renderWidth, int renderHeight, int viewportWidth, int viewportHeight) {
        this.renderWidth = renderWidth;
        this.renderHeight = renderHeight;
        this.viewportWidth = viewportWidth;
        this.viewportHeight = viewportHeight;
        Size size = calculateDisplaySize(viewportWidth, viewportHeight, renderWidth, renderHeight);
        displayWidth = size.getWidth();
        displayHeight = size.getHeight();
    }

    private Size calculateDisplaySize(int viewportWidth, int viewportHeight, int renderWidth, int renderHeight) {
        float ratioFrame = (float) renderWidth / renderHeight;
        RectF frameRect = MathUtil.getFitCenterRectFrame(viewportWidth, viewportHeight, ratioFrame, 0.0001f);
        return new Size((int) frameRect.width(), (int) frameRect.height());
    }

    public int draw(int textureId, int width, int height) {
        return textureId;
    }

    public void postDraw(Runnable runnable) {
        passContext.postDraw(runnable);
    }

    public void postDrawWithoutSwap(Runnable runnable) {
        passContext.postDrawWithoutSwap(runnable);
    }

    public void post(Runnable runnable) {
        passContext.post(runnable);
    }

    public void postRelease() {
        passContext.post(this::release);
    }

    @CallSuper
    public void release() {
    }

}
