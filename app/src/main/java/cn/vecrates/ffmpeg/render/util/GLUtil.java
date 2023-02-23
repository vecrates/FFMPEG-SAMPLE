package cn.vecrates.ffmpeg.render.util;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.graphics.Bitmap;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.opengl.Matrix;
import android.util.Log;

import java.math.BigDecimal;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import javax.microedition.khronos.opengles.GL10;

public class GLUtil {

    public static final float[] IDENTITY_MATRIX;
    public static final float[] TRANS_90_MATRIX;
    public static final float[] TRANS_180_MATRIX;
    public static final float[] TRANS_270_MATRIX;
    public static final float[] TRANS_N_90_MATRIX;

    public static final float[] VERTICAL_MIRROR_MATRIX;
    public static final float[] HORIZONTAL_MIRROR_MATRIX;
    public static final float[] HV_MIRROR_MATRIX;

    static {
        IDENTITY_MATRIX = new float[16];
        Matrix.setIdentityM(IDENTITY_MATRIX, 0);

        TRANS_90_MATRIX = new float[16];
        Matrix.setIdentityM(TRANS_90_MATRIX, 0);
        Matrix.rotateM(TRANS_90_MATRIX, 0, 90, 0, 0, 1);

        TRANS_N_90_MATRIX = new float[16];
        Matrix.setIdentityM(TRANS_N_90_MATRIX, 0);
        Matrix.rotateM(TRANS_N_90_MATRIX, 0, -90, 0, 0, 1);

        TRANS_180_MATRIX = new float[16];
        Matrix.setIdentityM(TRANS_180_MATRIX, 0);
        Matrix.rotateM(TRANS_180_MATRIX, 0, 180, 0, 0, 1);

        TRANS_270_MATRIX = new float[16];
        Matrix.setIdentityM(TRANS_270_MATRIX, 0);
        Matrix.rotateM(TRANS_270_MATRIX, 0, 270, 0, 0, -1);

        //垂直镜像
        VERTICAL_MIRROR_MATRIX = new float[]{
                1f, 0f, 0f, 0f,
                0f, -1f, 0f, 0f,
                0f, 0f, 1f, 0f,
                0f, 0f, 0f, 1f,
        };

        //水平镜像
        HORIZONTAL_MIRROR_MATRIX = new float[]{
                -1f, 0f, 0f, 0f,
                0f, 1f, 0f, 0f,
                0f, 0f, 1f, 0f,
                0f, 0f, 0f, 1f,
        };

        //水平垂直镜像
        HV_MIRROR_MATRIX = new float[]{
                -1f, 0f, 0f, 0f,
                0f, -1f, 0f, 0f,
                0f, 0f, 1f, 0f,
                0f, 0f, 0f, 1f,
        };
    }

    //顶点
    public final static float[] VERTEX_COORD = {
            -1.0f, -1.0f,   // 0 bottom left
            1.0f, -1.0f,   // 1 bottom right
            -1.0f, 1.0f,   // 2 top left
            1.0f, 1.0f,   // 3 top right
    };

    //纹理
    public final static float[] TEXTURE_COORD = {
            0.0f, 0.0f,     // 0 bottom left
            1.0f, 0.0f,     // 1 bottom right
            0.0f, 1.0f,     // 2 top left
            1.0f, 1.0f      // 3 top right
    };

    public final static float[] TEXTURE_COORD_INVERSE = {
            0, 1,
            1, 1,
            0, 0,
            1, 0,
    };

    public static final FloatBuffer VERTEX_BUFFER = createFloatBuffer(VERTEX_COORD);
    public static final FloatBuffer TEXTURE_BUFFER = createFloatBuffer(TEXTURE_COORD);
    public static final FloatBuffer TEXTURE_BUFFER_INVERSE = createFloatBuffer(TEXTURE_COORD_INVERSE);

    public static final int FLOAT_BYTES = Float.SIZE / Byte.SIZE;

    public static FloatBuffer createFloatBuffer(float[] coords) {
        return (FloatBuffer) ByteBuffer
                .allocateDirect(coords.length * FLOAT_BYTES)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(coords)
                .position(0);
    }

    public static ShortBuffer createShotBuffer(short[] coords) {
        return (ShortBuffer) ByteBuffer
                .allocateDirect(coords.length * Short.SIZE / Byte.SIZE)
                .order(ByteOrder.nativeOrder())
                .asShortBuffer()
                .put(coords)
                .position(0);

    }

    public static float[] newIdentityMatrix() {
        float[] matrix = new float[16];
        Matrix.setIdentityM(matrix, 0);
        return matrix;
    }

    public static float[] newVerticalMirrorMatrix() {
        float[] matrix = new float[16];
        Matrix.setIdentityM(matrix, 0);
        Matrix.scaleM(matrix, 0, 1, -1, 1);
        return matrix;
    }

    public static float[] setVerticalMirrorMatrix(float[] matrix) {
        Matrix.setIdentityM(matrix, 0);
        Matrix.scaleM(matrix, 0, 1, -1, 1);
        return matrix;
    }

    public static int genTexture(boolean isOes) {
        int[] textures = {0};
        int tryCount = 0;
        while (textures[0] <= 0 && tryCount < 10) {
            tryCount++;
            GLES20.glGenTextures(1, textures, 0);
        }
        int target = isOes ? GLES11Ext.GL_TEXTURE_EXTERNAL_OES : GLES20.GL_TEXTURE_2D;
        GLES20.glBindTexture(target, textures[0]);
        GLES20.glTexParameteri(target, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        GLES20.glTexParameteri(target, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        GLES20.glTexParameteri(target, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameteri(target, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);
        return textures[0];
    }


    public static int createProgram(String vertexShaderCode, String fragmentShaderCode) {
        int program = GLES20.glCreateProgram();
        int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexShaderCode);
        int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentShaderCode);
        GLES20.glAttachShader(program, vertexShader);
        GLES20.glAttachShader(program, fragmentShader);
        GLES20.glLinkProgram(program);
        GLES20.glDeleteShader(vertexShader);
        GLES20.glDeleteShader(fragmentShader);
        return program;
    }

    public static int loadShader(int type, String shaderCode) {
        int shader = GLES20.glCreateShader(type);
        GLES20.glShaderSource(shader, shaderCode);
        GLES20.glCompileShader(shader);
        return shader;
    }

    public static int loadTexture(Bitmap bitmap) {
        int[] textureHandle = {0};

        int tryCount = 0;
        while (textureHandle[0] <= 0 && tryCount < 10) {
            tryCount++;
            GLES20.glGenTextures(1, textureHandle, 0);
        }

        if (textureHandle[0] > 0) {
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureHandle[0]);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);
        }

        if (textureHandle[0] <= 0) {
            Log.d("GLUtil", "loadTexture: code=" + GLES20.glGetError());
            return -1;
        }

        return textureHandle[0];
    }


    public static int getMaxTextureSize() {
        int[] maxSize = new int[1];
        GLES20.glGetIntegerv(GLES20.GL_MAX_TEXTURE_SIZE, maxSize, 0);
        return maxSize[0];
    }

    public static Bitmap getBitmapFromFrameBuffer(int width, int height) {
        return getBitmapFromFrameBuffer(width, height, true);
    }

    public static Bitmap getBitmapFromFrameBuffer(int width, int height, boolean flip) {
        return getBitmapFromFrameBuffer(0, 0, width, height, flip);
    }

    public static Bitmap getBitmapFromFrameBuffer(int x, int y, int width, int height, boolean flip) {
        ByteBuffer pixelBuffer = ByteBuffer.allocateDirect(width * height * 4).order(ByteOrder.nativeOrder());
        GLES20.glReadPixels(x, y, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, pixelBuffer);
        Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        bitmap.copyPixelsFromBuffer(pixelBuffer);
        pixelBuffer.clear();
        //FrameBuffer读取的bitmap会上下颠倒
        if (flip) {
            bitmap = verticalFlip(bitmap);
        }
        return bitmap;
    }

    public static ByteBuffer readPixelsFromFrameBuffer(int width, int height) {
        ByteBuffer byteBuffer = ByteBuffer.allocateDirect(width * height * 4).order(ByteOrder.nativeOrder());
        GLES20.glReadPixels(0, 0, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, byteBuffer);
        return byteBuffer;
    }

    public static int readPixelFromFrameBuffer(int x, int y) {
        ByteBuffer byteBuffer = ByteBuffer.allocateDirect(4).order(ByteOrder.nativeOrder());
        GLES20.glReadPixels(x, y, 1, 1, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, byteBuffer);
        byte[] bytes = new byte[4];
        byteBuffer.get(bytes);
        int i = 0xff & bytes[2];
        i |= (bytes[1] << 8) & 0xff00;
        i |= (bytes[0] << 16) & 0xff0000;
        i |= (bytes[3] << 24) & 0xff000000;
        return i;
    }

    private static Bitmap verticalFlip(Bitmap bm) {
        android.graphics.Matrix m = new android.graphics.Matrix();
        m.setScale(1, -1);//垂直翻转
        int w = bm.getWidth();
        int h = bm.getHeight();
        //生成的翻转后的bitmap
        Bitmap finalBm = Bitmap.createBitmap(bm, 0, 0, w, h, m, true);
        if (!bm.isRecycled()) {
            bm.recycle();
        }
        return finalBm;
    }

    public static void deleteTextureId(int textureId) {
        int[] textureIds = new int[]{textureId};
        GLES20.glDeleteTextures(1, textureIds, 0);
    }

    public static boolean isSupportGL30(Context context) {
        ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        ConfigurationInfo info = am.getDeviceConfigurationInfo();
        return new BigDecimal(info.getGlEsVersion()).compareTo(new BigDecimal("3.0")) >= 0;
    }

}
