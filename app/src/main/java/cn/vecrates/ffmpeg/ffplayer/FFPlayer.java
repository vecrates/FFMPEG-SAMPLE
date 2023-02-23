package cn.vecrates.ffmpeg.ffplayer;

import android.util.Log;

import androidx.annotation.NonNull;

public class FFPlayer {

    private static final String TAG = "FFPlayer";

    static {
        System.loadLibrary("ffplayer");
    }

    private long nativeObj = -1;

    private FFDecodeListener decodeListener;

    public FFPlayer() {
        nativeObj = nativeCreate();
        if (nativeObj > 0) {
            nativeSetListener(nativeObj, jniListener); //todo 主线程？
        }
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

    public void stop() {
        if (nativeObj < 0) {
            Log.e(TAG, "stop: illegal native object");
            return;
        }
        nativeStop(nativeObj);
    }

    @NonNull
    public int[] getVideoSize() {
        if (nativeObj < 0) {
            Log.e(TAG, "getVideoSize: illegal native object");
            return new int[]{0, 0};
        }
        return nativeGetVideoSize(nativeObj);
    }

    public void setDecodeListener(FFDecodeListener decodeListener) {
        this.decodeListener = decodeListener;
    }

    //callback

    private final JniListener jniListener = new JniListener() {
        @Override
        public void onVideoFrameAvailable(byte[] y, byte[] u, byte[] v) {
            Log.e(TAG, "onVideoFrameAvailable: " + y.length);
            if (decodeListener != null) {
                decodeListener.onVideoFrameAvailable(y, u, v);
            }
        }

        @Override
        public void onAudioFrameAvailable(byte[] pcmArray) {
            if (decodeListener != null) {
                decodeListener.onAudioFrameAvailable(pcmArray);
            }
        }
    };

    private native void nativeSetListener(long obj, JniListener listener);

    private native long nativeCreate();

    private native void nativeDestroy(long obj);

    private native boolean nativePrepare(long obj, String file);

    private native void nativeReset(long obj);

    private native void nativeStart(long obj);

    private native void nativeStop(long obj);

    private native int[] nativeGetVideoSize(long obj);


    private interface JniListener {
        void onVideoFrameAvailable(byte[] y, byte[] u, byte[] v);

        void onAudioFrameAvailable(byte[] pcmArray);
    }

}
