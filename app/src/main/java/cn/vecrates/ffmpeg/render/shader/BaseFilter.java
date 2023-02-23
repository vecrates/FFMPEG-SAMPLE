package cn.vecrates.ffmpeg.render.shader;


import android.graphics.PointF;
import android.opengl.GLES20;

import java.nio.FloatBuffer;

import cn.vecrates.ffmpeg.render.util.GLUtil;

public class BaseFilter {

    protected int program = -1;

    public BaseFilter() {

    }

    protected final void init(String vsCode, String fsCode) {
        program = GLUtil.createProgram(vsCode, fsCode);
        onInit();
    }

    protected void onInit() {

    }

    public void setInteger(int location, final int intValue) {
        GLES20.glUniform1i(location, intValue);
    }

    public void setFloat(int location, final float floatValue) {
        GLES20.glUniform1f(location, floatValue);
    }

    public void setPoint(int location, PointF point) {
        float[] vec2 = new float[2];
        vec2[0] = point.x;
        vec2[1] = point.y;
        GLES20.glUniform2fv(location, 1, vec2, 0);
    }

    public void setFloatVec2(int location, float[] arrayValue) {
        GLES20.glUniform2fv(location, 1, FloatBuffer.wrap(arrayValue));
    }

    public void setFloatVec2(int location, FloatBuffer floatBuffer) {
        GLES20.glUniform2fv(location, 1, floatBuffer);
    }

    public void setFloatVec3(int location, float[] arrayValue) {
        GLES20.glUniform3fv(location, 1, FloatBuffer.wrap(arrayValue));
    }

    public void setUniformMatrix4f(int location, final float[] matrix) {
        GLES20.glUniformMatrix4fv(location, 1, false, matrix, 0);
    }

    public void release() {
        if (program != -1) {
            GLES20.glDeleteProgram(program);
            program = -1;
        }
    }

}
