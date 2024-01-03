package cn.vecrates.ffmpeg.ffmpeg;

public class FFEncoder {

    private long obj = -1;

    public FFEncoder() {
        obj = nativeCreate();
    }

    public void destroy() {
        if (obj == -1) {
            return;
        }
        nativeDestroy(obj);
        obj = -1;
    }

    public void start() {
        if (obj == -1) {
            return;
        }
        nativeStart(obj);
    }

    public void stop() {
        if (obj == -1) {
            return;
        }
        nativeStop(obj);
    }

    public void reset() {
        if (obj == -1) {
            return;
        }
        nativeReset(obj);
    }

    public boolean init(String src, String dest) {
        if (obj == -1) {
            return false;
        }
        return nativeInit(obj, src, dest);
    }

    private native long nativeCreate();

    private native void nativeDestroy(long obj);

    private native void nativeStart(long obj);

    private native void nativeStop(long obj);

    private native void nativeReset(long obj);

    private native boolean nativeInit(long obj, String src, String dest);

}
