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
        nativeSetListener(nativeObj, jniListener); //todo 主线程？
        nativePrepare(nativeObj, file);
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

    //callback

    private final JniListener jniListener = new JniListener() {
        @Override
        public void onVideoFrameAvailable(byte[] y, byte[] u, byte[] v) {

        }

        @Override
        public void onAudioFrameAvailable(byte[] pcmArray) {

        }
    };

    private native void nativeSetListener(long obj, Object listener);

    private native long nativeCreate();

    private native void nativeDestroy(long obj);

    private native boolean nativePrepare(long obj, String file);

    private native void nativeReset(long obj);

    private native void nativeStart(long obj);

    private native void nativeStop(long obj);


    private interface JniListener {
        void onVideoFrameAvailable(byte[] y, byte[] u, byte[] v);

        void onAudioFrameAvailable(byte[] pcmArray);

    }

}
