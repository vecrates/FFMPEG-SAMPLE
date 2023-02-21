package cn.vecrates.ffmpeg.render;

import android.opengl.EGLContext;
import android.util.Size;

import androidx.annotation.NonNull;

/**
 * 为渲染端提供控制端的gl环境等
 * 渲染端（专注渲染的内容）
 * 控制端（管理gl环境及控制渲染的调用等）
 */
public interface IGLThreadProxy {

    void post(@NonNull Runnable r);

    void postDraw(@NonNull Runnable r);

    EGLContext getGLContext();

    @NonNull
    Size getSurfaceSize();

}
