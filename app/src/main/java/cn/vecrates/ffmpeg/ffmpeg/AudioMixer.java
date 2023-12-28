package cn.vecrates.ffmpeg.ffmpeg;

import androidx.annotation.Nullable;

public class AudioMixer {

    private long instance = -1;

    public AudioMixer() {
        instance = nativeCreate();
    }

    public void destroy() {
        if (instance != -1) {
            nativeDestroy(instance);
            instance = -1;
        }
    }

    public boolean addAudio(String path) {
        if (instance == -1) {
            return false;
        }
        return nativeAddAudio(instance, path);
    }

    public boolean init() {
        if (instance == -1) {
            return false;
        }
        return nativeInit(instance);
    }

    @Nullable
    public byte[] readFrame() {
        if (instance == -1) {
            return null;
        }
        return readFrame(instance);
    }

    private native long nativeCreate();

    private native void nativeDestroy(long obj);

    private native boolean nativeAddAudio(long obj, String path);

    private native boolean nativeInit(long obj);

    private native byte[] readFrame(long obj);

}
