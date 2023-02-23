package cn.vecrates.ffmpeg.render.shader;


import android.opengl.GLES20;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;

import cn.vecrates.ffmpeg.render.util.GLUtil;


public class YUVFilter extends BaseFilter {

    protected static final String VERTEX_CODE = "attribute vec4 position;\n" +
            "attribute vec2 inputTextureCoordinate;\n" +
            "varying vec2 textureCoordinate;\n" +
            "uniform mat4 vertexMatrix;\n" +
            "uniform mat4 textureMatrix;\n" +
            "void main()\n" +
            "{\n" +
            "    gl_Position = vertexMatrix * position;\n" +
            "    textureCoordinate = (textureMatrix * vec4(inputTextureCoordinate, 0.0, 1.0)).xy;" +
            "}";

    protected static final String FRAGMENT_CODE = "precision highp float;\n" +
            "\n" +
            "varying  vec2 textureCoordinate;\n" +
            "uniform sampler2D yTexture;\n" +
            "uniform sampler2D uTexture;\n" +
            "uniform sampler2D vTexture;\n" +
            "\n" +
            "void main() {\n" +
            "    float y = texture2D(yTexture, textureCoordinate).r;//[0,1]\n" +
            "    float u = texture2D(uTexture, textureCoordinate).r - 0.5;//[-0.5~,0.5]\n" +
            "    float v = texture2D(vTexture, textureCoordinate).r - 0.5;\n" +
            "\n" +
            "    //BT.601 全色域，YVUJ402P\n" +
            "    vec3 rgb;\n" +
            "    rgb.r = y + 1.403 * v;\n" +
            "    rgb.g = y - 0.344 * u - 0.714 * v;\n" +
            "    rgb.b = y + 1.770 * u;\n" +
            "\n" +
            "    gl_FragColor = vec4(rgb, 1.0);\n" +
            "}";

    private int texMatrixLoc;
    private int vertMatrixLoc;
    private int positionLoc;
    private int texCoordinateLoc;
    private int yTextureLoc;
    private int uTextureLoc;
    private int vTextureLoc;

    private final float[] vertCoordinates = {
            -1.0f, -1.0f,  // 0 lb
            1.0f, -1.0f,   // 1 rb
            -1.0f, 1.0f,   // 2 lt
            1.0f, 1.0f,    // 3 rt
    };

    private final float[] texCoordinates = {
            0.0f, 0.0f,     // 0 lb
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
        init(VERTEX_CODE, FRAGMENT_CODE);
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

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, yTextureId);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE,
                width, height, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, yBuffer);
        GLES20.glUniform1i(yTextureLoc, 0);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, uTextureId);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE,
                width / 2, height / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, uBuffer);
        GLES20.glUniform1i(uTextureLoc, 1);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE2);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, vTextureId);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE,
                width / 2, height / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, vBuffer);
        GLES20.glUniform1i(vTextureLoc, 2);

        GLES20.glUniformMatrix4fv(texMatrixLoc, 1, false, GLUtil.IDENTITY_MATRIX, 0);
        GLES20.glUniformMatrix4fv(vertMatrixLoc, 1, false, GLUtil.IDENTITY_MATRIX, 0);

        GLES20.glEnableVertexAttribArray(positionLoc);
        GLES20.glVertexAttribPointer(positionLoc, 2, GLES20.GL_FLOAT, false,
                2 * GLUtil.FLOAT_BYTES, vertCoordinateBuffer);

        GLES20.glEnableVertexAttribArray(texCoordinateLoc);
        GLES20.glVertexAttribPointer(texCoordinateLoc, 2, GLES20.GL_FLOAT, false,
                2 * GLUtil.FLOAT_BYTES, texCoordinateBuffer);

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
