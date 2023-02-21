package cn.vecrates.ffmpeg.render;

import android.graphics.SurfaceTexture;

/**
 * 继承渲染基类 + 针对视频渲染的接口
 */
public abstract class AbsVideoDrawerProxy extends AbsDrawerProxy {

    public abstract int getRenderTexture();

    public abstract void updatedImageTex(SurfaceTexture surfaceTexture);


}
