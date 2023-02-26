package cn.vecrates.ffmpeg.render.pass;


import android.opengl.GLES20;
import android.util.Log;

import java.nio.ByteBuffer;

import cn.vecrates.ffmpeg.render.PassContext;
import cn.vecrates.ffmpeg.render.common.GLFrameBuffer;
import cn.vecrates.ffmpeg.render.shader.YUVFilter;

public class FormatPass extends BasePass {

    private GLFrameBuffer glFrameBuffer;
    private YUVFilter yuvFilter;

    private ByteBuffer yBuffer;
    private ByteBuffer uBuffer;
    private ByteBuffer vBuffer;

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
        glFrameBuffer = new GLFrameBuffer();
        yuvFilter = new YUVFilter();
    }

    /**
     * call on gl
     */
    public void updateYUV(byte[] y, byte[] u, byte[] v) {
        Log.e("$$$$", "updateYUV: " +
                "y=" + y.length + " u=" + u.length + " v=" + v.length);
        yBuffer = ByteBuffer.wrap(y);
        uBuffer = ByteBuffer.wrap(u);
        vBuffer = ByteBuffer.wrap(v);
    }

    @Override
    public int draw(int textureId, int width, int height) {

        glFrameBuffer.bindFrameBuffer(width, height);
        GLES20.glViewport(0, 0, width, height);
        yuvFilter.draw(yBuffer, uBuffer, vBuffer, width, height);
        glFrameBuffer.unBindFrameBuffer();
        textureId = glFrameBuffer.getAttachedTexture();

        return textureId;
    }

    @Override
    public void release() {
        super.release();
        if (glFrameBuffer != null) {
            glFrameBuffer.destroyFrameBuffer();
            glFrameBuffer = null;
        }
        if (yuvFilter != null) {
            yuvFilter.release();
            yuvFilter = null;
        }
    }

}
