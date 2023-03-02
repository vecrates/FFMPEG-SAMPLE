package cn.vecrates.ffmpeg.util;

import android.os.Handler;
import android.os.Looper;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public class ThreadHelper {

    private static Handler handler;

    private final static ExecutorService executor = new ThreadPoolExecutor(5, Integer.MAX_VALUE,
            2L, TimeUnit.SECONDS, new SynchronousQueue<>());

    public static void runBackground(Runnable runnable) {
        executor.execute(runnable);
    }

    public static void runOnUIThread(Runnable runnable, long delay) {
        if (handler == null) {
            handler = new Handler(Looper.getMainLooper());
        }
        handler.postDelayed(runnable, delay);
    }

    public static void runOnUIThread(Runnable runnable) {
        runOnUIThread(runnable, 0);
    }

    public static boolean isMainThread() {
        return Looper.getMainLooper().getThread() == Thread.currentThread();
    }

}
