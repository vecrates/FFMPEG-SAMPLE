package cn.vecrates.ffmpeg.ffplayer;

import android.util.Log;
import android.view.Surface;

public class FFPlayer {

    private static final String TAG = "FFPlayer";

    static {
        System.loadLibrary("ffplayer");
    }

    private long nativeObj = -1;

    public FFPlayer() {
        nativeObj = nativeCreate();
    }

    public void release() {
        if (nativeObj < 0) {
            Log.e(TAG, "release: illegal native object");
            return;
        }
        nativeDestroy(nativeObj);
        nativeObj = -1;
    }

    public void prepare(String file) {
        if (nativeObj < 0) {
            Log.e(TAG, "prepare: illegal native object");
            return;
        }
        nativePrepare(nativeObj, file);
    }

    public void setSurface(Surface surface, int width, int height) {
        if (nativeObj < 0) {
            Log.e(TAG, "setSurface: illegal native object");
            return;
        }
        nativeSetSurface(nativeObj, surface, width, height);
    }

    public void reset() {
        if (nativeObj < 0) {
            Log.e(TAG, "reset: illegal native object");
            return;
        }
        nativeReset(nativeObj);
    }

    public void start() {
        if (nativeObj < 0) {
            Log.e(TAG, "play: illegal native object");
            return;
        }
        nativeStart(nativeObj);
    }

    private native long nativeCreate();

    private native void nativeDestroy(long obj);

    private native boolean nativePrepare(long obj, String file);

    private native void nativeSetSurface(long obj, Surface surface, int width, int height);

    private native void nativeReset(long obj);

    private native void nativeStart(long obj);

    private native void nativeStop(long obj);

}
