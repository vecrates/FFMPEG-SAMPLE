package cn.vecrates.ffmpeg.ffmpeg;

import androidx.annotation.Nullable;

public class FFmpegCmd {

    public static void release() {
        nativeRelease();
    }

    public static int exec(String[] cmds,
                           @Nullable CmdCallback callback) {
        return nativeExe(cmds, callback);
    }

    private static native int nativeExe(String[] cmds,
                                        @Nullable CmdCallback callback);

    private static native void nativeRelease();

    public interface CmdCallback {
        void onProgress(long currentUs, long durationUs);
    }

}
