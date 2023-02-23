package cn.vecrates.ffmpeg.render.shader;


import android.opengl.GLES20;
import android.util.Log;

import java.nio.FloatBuffer;

import cn.vecrates.ffmpeg.render.util.GLUtil;


public class ShowFilter extends BaseFilter {

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
            "varying vec2 textureCoordinate;\n" +
            "uniform sampler2D inputTexture;\n" +
            "\n" +
            "void main() {\n" +
            "    gl_FragColor = texture2D(inputTexture, textureCoordinate);\n" +
            "}";

    private int texMatrixLoc;
    private int vertMatrixLoc;
    private int positionLoc;
    private int texCoordinateLoc;
    private int inputTextureLoc;

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

    public ShowFilter() {
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
        inputTextureLoc = GLES20.glGetUniformLocation(program, "inputTexture");

        vertCoordinateBuffer = GLUtil.createFloatBuffer(vertCoordinates);
        texCoordinateBuffer = GLUtil.createFloatBuffer(texCoordinates);
    }

    public void setVertCoordinate(float[] vertCoordinate) {
        vertCoordinateBuffer.clear();
        vertCoordinateBuffer.put(vertCoordinate);
        vertCoordinateBuffer.position(0);
    }

    public void setTexCoordinate(float[] texCoordinate) {
        texCoordinateBuffer.clear();
        texCoordinateBuffer.put(texCoordinate);
        texCoordinateBuffer.position(0);
    }

    public void draw(int textureId, float[] vertexMatrix, float[] texMatrix) {
        if (texMatrix == null) {
            texMatrix = GLUtil.IDENTITY_MATRIX;
        }
        if (vertexMatrix == null) {
            vertexMatrix = GLUtil.IDENTITY_MATRIX;
        }

        GLES20.glClearColor(0, 0, 0, 0);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

        GLES20.glUseProgram(program);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);
        GLES20.glUniform1i(inputTextureLoc, 0);

        GLES20.glUniformMatrix4fv(texMatrixLoc, 1, false, texMatrix, 0);
        GLES20.glUniformMatrix4fv(vertMatrixLoc, 1, false, vertexMatrix, 0);

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

}
