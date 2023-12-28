//
// Created by weiyusong on 2023/2/8.
//

#include <jni.h>
#include <android/native_window_jni.h>

#include "decoder/FFPlayer.h"
#include "tool/AudioMixer.h"
#include "util/StringUtil.h"

/*************FFPlayer************/

extern "C"
JNIEXPORT jlong JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFPlayer_nativeCreate(JNIEnv *env, jobject instance) {
    auto *ffPlayer = new FFPlayer();
    return (jlong) ffPlayer;
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFPlayer_nativeDestroy(JNIEnv *env, jobject instance, jlong obj) {
    auto *ffPlayer = (FFPlayer *) obj;
    ffPlayer->reset();
    delete ffPlayer;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFPlayer_nativePrepare(JNIEnv *env, jobject instance,
                                                      jlong obj,
                                                      jstring jFile) {
    std::string fileName = str_u::jstring2string(env, jFile);
    auto *ffPlayer = (FFPlayer *) obj;
    return ffPlayer->prepare(env, fileName);
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFPlayer_nativeSetListener(JNIEnv *env, jobject instance,
                                                          jlong obj,
                                                          jobject jListener) {
    auto *ffPlayer = (FFPlayer *) obj;
    ffPlayer->setJNIListenContext(env, jListener);
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFPlayer_nativeReset(JNIEnv *env, jobject instance, jlong obj) {
    auto *ffPlayer = (FFPlayer *) obj;
    ffPlayer->reset();
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFPlayer_nativeStart(JNIEnv *env, jobject instance, jlong obj) {
    auto *ffPlayer = (FFPlayer *) obj;
    ffPlayer->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFPlayer_nativeStop(JNIEnv *env, jobject instance, jlong obj) {
    auto *ffPlayer = (FFPlayer *) obj;
    ffPlayer->stop();
}

extern "C"
JNIEXPORT jintArray JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFPlayer_nativeGetVideoSize(JNIEnv *env, jobject instance,
                                                           jlong obj) {
    auto *ffPlayer = (FFPlayer *) obj;
    int *size = ffPlayer->getVideoSize();
    jintArray jSize = env->NewIntArray(2);
    jint jIntBuf[] = {size[0], size[1]};
    env->SetIntArrayRegion(jSize, 0, 2, jIntBuf);
    return jSize;
}

/*************FFPlayer end************/


/*************AudioMixer************/

extern "C"
JNIEXPORT jlong JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_AudioMixer_nativeCreate(JNIEnv *env, jobject instance) {
    auto *audioMixer = new AudioMixer();
    return (jlong) audioMixer;
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_AudioMixer_nativeDestroy(JNIEnv *env, jobject instance,
                                                        jlong obj) {
    auto *audioMixer = (AudioMixer *) obj;
    audioMixer->reset();
    delete audioMixer;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_AudioMixer_nativeAddAudio(JNIEnv *env, jobject instance,
                                                         jlong obj,
                                                         jstring filePath_) {
    const char *filePath = env->GetStringUTFChars(filePath_, 0);
    auto *audioMixer = (AudioMixer *) obj;
    bool result = audioMixer->addAudio(filePath);
    env->ReleaseStringUTFChars(filePath_, filePath);
    return result;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_AudioMixer_nativeInit(JNIEnv *env, jobject instance,
                                                     jlong obj) {
    auto *audioMixer = (AudioMixer *) obj;
    return audioMixer->init();
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_AudioMixer_readFrame(JNIEnv *env, jobject instance,
                                                    jlong obj) {
    auto *audioMixer = (AudioMixer *) obj;
    AudioPCM *pAudioPcm = audioMixer->readFrame();
    jbyteArray jPCM = env->NewByteArray(pAudioPcm->size);
    env->SetByteArrayRegion(jPCM, 0, pAudioPcm->size,
                            (const jbyte *) pAudioPcm->ptr);
    delete pAudioPcm;
    return jPCM;
}


/*************AudioMixer end************/