//
// Created by weiyusong on 2023/2/8.
//

#include <jni.h>
#include <android/native_window_jni.h>

#include "decoder/FFPlayer.h"
#include "util/StringUtil.h"

extern "C"
JNIEXPORT jlong JNICALL
Java_cn_vecrates_ffmpeg_ffplayer_FFPlayer_nativeCreate(JNIEnv *env, jobject instance) {
    auto *ffPlayer = new FFPlayer();
    return (jlong) ffPlayer;
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffplayer_FFPlayer_nativeDestroy(JNIEnv *env, jobject instance, jlong obj) {
    auto *ffPlayer = (FFPlayer *) obj;
    ffPlayer->reset();
    delete ffPlayer;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_cn_vecrates_ffmpeg_ffplayer_FFPlayer_nativePrepare(JNIEnv *env, jobject instance,
                                                        jlong obj,
                                                        jstring jFile) {
    std::string fileName = str_u::jstring2string(env, jFile);
    auto *ffPlayer = (FFPlayer *) obj;
    return ffPlayer->prepare(fileName);
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffplayer_FFPlayer_nativeSetSurface(JNIEnv *env, jobject instance,
                                                           jlong obj,
                                                           jobject jSurface,
                                                           jint width,
                                                           jint height) {
    auto *ffPlayer = (FFPlayer *) obj;
    if (jSurface == nullptr) {
        ffPlayer->setNativeWindow(nullptr, 0, 0);
        return;
    }
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, jSurface);
    ffPlayer->setNativeWindow(nativeWindow, width, height);
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffplayer_FFPlayer_nativeReset(JNIEnv *env, jobject instance, jlong obj) {
    auto *ffPlayer = (FFPlayer *) obj;
    ffPlayer->reset();
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffplayer_FFPlayer_nativeStart(JNIEnv *env, jobject instance, jlong obj) {
    auto *ffPlayer = (FFPlayer *) obj;
    ffPlayer->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffplayer_FFPlayer_nativeStop(JNIEnv *env, jobject instance, jlong obj) {
    auto *ffPlayer = (FFPlayer *) obj;
    ffPlayer->stop();
}