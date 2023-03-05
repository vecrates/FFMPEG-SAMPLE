package cn.vecrates.ffmpeg.render.pass;


import android.opengl.GLES20;
import android.util.Log;

import java.nio.ByteBuffer;

import cn.vecrates.ffmpeg.render.PassContext;
import cn.vecrates.ffmpeg.render.common.GLFrameBuffer;
import cn.vecrates.ffmpeg.render.shader.ShowFilter;
import cn.vecrates.ffmpeg.render.shader.YUVFilter;

public class FormatPass extends BasePass {

    private YUVFilter yuvFilter;
    private ShowFilter showFilter;

    private ByteBuffer yBuffer;
    private ByteBuffer uBuffer;
    private ByteBuffer vBuffer;

    private int yuvWidth = -1;
    private int yuvHeight = -1;

    /**
     * call on GL
     */
    public FormatPass(PassContext passContext) {
        super(passContext);
        init();
    }

    @Override
    protected void init() {
        super.init();
        yuvFilter = new YUVFilter();
        showFilter = new ShowFilter();
    }

    /**
     * call on gl
     */
    public void updateYUV(byte[] y, byte[] u, byte[] v, int width, int height) {
//        Log.e("$$$$", "updateYUV: " +
//                "y=" + y.length + " u=" + u.length + " v=" + v.length);
        yBuffer = ByteBuffer.wrap(y);
        uBuffer = ByteBuffer.wrap(u);
        vBuffer = ByteBuffer.wrap(v);
        yuvWidth = width;
        yuvHeight = height;
    }

    @Override
    public int draw(int textureId, int width, int height) {

        if (yBuffer == null) {
            return textureId;
        }

        GLFrameBuffer frameBuffer = passContext.nextFrameBuffer();
        frameBuffer.bindFrameBuffer(yuvWidth, yuvHeight);
        GLES20.glViewport(0, 0, yuvWidth, yuvHeight);
        yuvFilter.draw(yBuffer, uBuffer, vBuffer, yuvWidth, yuvHeight);
        frameBuffer.unBindFrameBuffer();
        textureId = frameBuffer.getAttachedTexture();


        float right = (float) width / yuvWidth;
        Log.e("!!!!!", "draw: " + right);
        float[] texCoordinate = new float[]{
                0.0f, 0.0f,     // 0 lb
                right, 0.0f,     // 1 rb
                0.0f, 1.0f,     // 2 lt
                right, 1.0f      // 3 rt
        };
        showFilter.setTexCoordinate(texCoordinate);
        frameBuffer = passContext.nextFrameBuffer();
        frameBuffer.bindFrameBuffer(width, height);
        GLES20.glViewport(0, 0, width, height);
        showFilter.draw(textureId, null, null);
        frameBuffer.unBindFrameBuffer();
        textureId = frameBuffer.getAttachedTexture();

        return textureId;
    }

    @Override
    public void release() {
        super.release();
        if (showFilter != null) {
            showFilter.release();
            showFilter = null;
        }
        if (yuvFilter != null) {
            yuvFilter.release();
            yuvFilter = null;
        }
    }

}
