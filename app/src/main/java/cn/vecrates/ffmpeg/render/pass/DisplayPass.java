package cn.vecrates.ffmpeg.render.pass;

import android.opengl.GLES20;

import androidx.annotation.NonNull;

import cn.vecrates.ffmpeg.render.PassContext;
import cn.vecrates.ffmpeg.render.shader.ShowFilter;
import cn.vecrates.ffmpeg.render.util.GLUtil;
import cn.vecrates.ffmpeg.render.util.MathUtil;

public class DisplayPass extends BasePass {

    private  ShowFilter showFilter;

    public DisplayPass(@NonNull PassContext passContext) {
        super(passContext);
        init();
    }

    @Override
    protected void init() {
        super.init();
        showFilter = new ShowFilter();
    }

    @Override
    public void onSizeChanged(int renderWidth, int renderHeight, int viewportWidth, int viewportHeight) {
        super.onSizeChanged(renderWidth, renderHeight, viewportWidth, viewportHeight);
        updateDisplayVertexes();
    }

    protected void updateDisplayVertexes() {
        float width = (float) displayWidth / viewportWidth;
        float height = (float) displayHeight / viewportHeight;
        float left = ((viewportWidth - displayWidth) * 0.5f) / viewportWidth;
        float top = ((viewportHeight - displayHeight) * 0.5f) / viewportHeight;
        float right = left + width;
        float bottom = top + height;
        left = MathUtil.range(left, -1, 1);
        top = MathUtil.range(top, -1, 1);
        right = MathUtil.range(right, -1, 1);
        bottom = MathUtil.range(bottom, -1, 1);
        if (showFilter != null) {
            showFilter.setVertCoordinate(new float[]{
                    left, top, //opengl坐标
                    right, top,
                    left, bottom,
                    right, bottom,
            });
        }
    }

    @Override
    public int draw(int textureId, int width, int height) {

        GLES20.glViewport(0, 0, viewportWidth, viewportHeight);
        showFilter.draw(textureId, GLUtil.VERTICAL_MIRROR_MATRIX, null);

        return textureId;
    }

    @Override
    public void release() {
        super.release();
        if (showFilter != null) {
            showFilter.release();
            showFilter = null;
        }
    }
}
