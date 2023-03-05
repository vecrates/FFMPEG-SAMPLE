package cn.vecrates.ffmpeg.render;

public abstract class AbsVideoDrawerProxy extends AbsDrawerProxy {

    abstract void updateYUV(byte[] y, byte[] u, byte[] v, int width, int height);

}
