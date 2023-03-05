package cn.vecrates.ffmpeg.render;

import android.opengl.EGLContext;
import android.util.Size;

import androidx.annotation.NonNull;

import cn.vecrates.ffmpeg.render.common.GLFrameBuffer;

public interface PassContext {

    void postDraw(Runnable runnable);

    void post(Runnable runnable);

    EGLContext getGLContext();

    @NonNull
    Size getSurfaceSize();

    GLFrameBuffer nextFrameBuffer();

}
