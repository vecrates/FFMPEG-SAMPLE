package cn.vecrates.ffmpeg.render.shader;


import android.opengl.GLES20;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;

import cn.vecrates.ffmpeg.render.util.GLUtil;
import cn.vecrates.ffmpeg.render.util.ShaderUtil;


public class YUVFilter extends BaseFilter {
    private int texMatrixLoc;
    private int vertMatrixLoc;
    private int positionLoc;
    private int texCoordinateLoc;
    private int yTextureLoc;
    private int uTextureLoc;
    private int vTextureLoc;

    private final float[] vertCoordinates = {-1.0f, -1.0f,  // 0 lb
            1.0f, -1.0f,   // 1 rb
            -1.0f, 1.0f,   // 2 lt
            1.0f, 1.0f,    // 3 rt
    };

    private final float[] texCoordinates = {0.0f, 0.0f,     // 0 lb
            1.0f, 0.0f,     // 1 rb
            0.0f, 1.0f,     // 2 lt
            1.0f, 1.0f      // 3 rt
    };

    private FloatBuffer vertCoordinateBuffer;
    private FloatBuffer texCoordinateBuffer;

    private int yTextureId;
    private int uTextureId;
    private int vTextureId;

    public YUVFilter() {
        super();
        init(ShaderUtil.readByAssets("shader/base_vertex.glsl"),
                ShaderUtil.readByAssets("shader/yuv_fragment.glsl"));
    }

    @Override
    protected void onInit() {
        super.onInit();
        positionLoc = GLES20.glGetAttribLocation(program, "position");
        texCoordinateLoc = GLES20.glGetAttribLocation(program, "inputTextureCoordinate");
        texMatrixLoc = GLES20.glGetUniformLocation(program, "textureMatrix");
        vertMatrixLoc = GLES20.glGetUniformLocation(program, "vertexMatrix");
        yTextureLoc = GLES20.glGetUniformLocation(program, "yTexture");
        uTextureLoc = GLES20.glGetUniformLocation(program, "uTexture");
        vTextureLoc = GLES20.glGetUniformLocation(program, "vTexture");

        vertCoordinateBuffer = GLUtil.createFloatBuffer(vertCoordinates);
        texCoordinateBuffer = GLUtil.createFloatBuffer(texCoordinates);

        yTextureId = GLUtil.genTexture(false);
        uTextureId = GLUtil.genTexture(false);
        vTextureId = GLUtil.genTexture(false);

    }

    public void draw(ByteBuffer yBuffer, ByteBuffer uBuffer, ByteBuffer vBuffer, int width, int height) {

        GLES20.glClearColor(0, 0, 0, 0);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

        GLES20.glUseProgram(program);

        yBuffer.position(0);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, yTextureId);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, width, height, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, yBuffer);
        GLES20.glUniform1i(yTextureLoc, 0);

        uBuffer.position(0);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, uTextureId);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, width / 2, height / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, uBuffer);
        GLES20.glUniform1i(uTextureLoc, 1);

        vBuffer.position(0);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE2);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, vTextureId);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, width / 2, height / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, vBuffer);
        GLES20.glUniform1i(vTextureLoc, 2);

        GLES20.glUniformMatrix4fv(texMatrixLoc, 1, false, GLUtil.IDENTITY_MATRIX, 0);
        GLES20.glUniformMatrix4fv(vertMatrixLoc, 1, false, GLUtil.IDENTITY_MATRIX, 0);

        GLES20.glEnableVertexAttribArray(positionLoc);
        GLES20.glVertexAttribPointer(positionLoc, 2, GLES20.GL_FLOAT, false, 2 * GLUtil.FLOAT_BYTES, vertCoordinateBuffer);

        GLES20.glEnableVertexAttribArray(texCoordinateLoc);
        GLES20.glVertexAttribPointer(texCoordinateLoc, 2, GLES20.GL_FLOAT, false, 2 * GLUtil.FLOAT_BYTES, texCoordinateBuffer);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        GLES20.glDisableVertexAttribArray(positionLoc);
        GLES20.glDisableVertexAttribArray(texCoordinateLoc);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);

        GLES20.glUseProgram(0);

        int code = GLES20.glGetError();
        if (code != 0) {
            Log.e(getClass().getSimpleName(), "error code=" + code);
        }

    }

    @Override
    public void release() {
        super.release();
        int[] textureIds = new int[]{yTextureId, uTextureId, vTextureId};
        GLES20.glDeleteTextures(textureIds.length, textureIds, 0);
    }
}
