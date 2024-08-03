//
// Created by weiyusong on 2023/2/8.
//

#include <jni.h>
#include <android/native_window_jni.h>

#include "decoder/FFPlayer.h"
#include "tool/AudioMixer.h"
#include "util/StringUtil.h"
#include "encoder/FFEncoder.h"

#ifdef __cplusplus
extern "C" { //ffmpeg用c语言的方式编译
#endif

#include "ffmpeg.h"

#ifdef __cplusplus
}
#endif


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
extern "C"
JNIEXPORT jlong JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFEncoder_nativeCreate(JNIEnv *env, jobject instance) {
    auto *ffEncoder = new FFEncoder();
    return (jlong) ffEncoder;
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFEncoder_nativeDestroy(JNIEnv *env, jobject instance,
                                                       jlong obj) {
    auto *ffEncoder = (FFEncoder *) obj;
    ffEncoder->reset();
    delete ffEncoder;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFEncoder_nativeInit(JNIEnv *env, jobject instance,
                                                    jlong obj,
                                                    jstring srcFile,
                                                    jstring outFile) {
    const char *srcPath_ = env->GetStringUTFChars(srcFile, 0);
    const char *outPath_ = env->GetStringUTFChars(outFile, 0);
    auto *ffEncoder = (FFEncoder *) obj;
    bool result = ffEncoder->init(env, srcPath_, outPath_);
    env->ReleaseStringUTFChars(srcFile, srcPath_);
    env->ReleaseStringUTFChars(outFile, outPath_);
    return result;
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFEncoder_nativeStart(JNIEnv *env, jobject instance,
                                                     jlong obj) {
    auto *ffEncoder = (FFEncoder *) obj;
    ffEncoder->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFEncoder_nativeStop(JNIEnv *env, jobject instance,
                                                    jlong obj) {
    auto *ffEncoder = (FFEncoder *) obj;
    ffEncoder->stop();
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFEncoder_nativeReset(JNIEnv *env, jobject instance,
                                                     jlong obj) {
    auto *ffEncoder = (FFEncoder *) obj;
    ffEncoder->reset();
}


/*************FFmpeg cmd************/

extern "C"
JNIEXPORT jint JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFmpegCmd_nativeExe(JNIEnv *env, jclass clazz,
                                                   jobjectArray jCmds,
                                                   jobject jCallback) {
    setCmdCallback(env, jCallback);
    int length = env->GetArrayLength(jCmds);
    const char **cmds = new const char *[length];
    for (int i = 0; i < length; i++) {
        auto item = (jstring) env->GetObjectArrayElement(jCmds, i);
        const char *cmdItem = env->GetStringUTFChars(item, 0);
        cmds[i] = cmdItem;
    }
    int result = exe_cmd(length, cmds);
    for (int i = 0; i < length; i++) {
        auto item = (jstring) env->GetObjectArrayElement(jCmds, i);
        env->ReleaseStringUTFChars(item, cmds[i]);
    }
    return result;
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_ffmpeg_FFmpegCmd_nativeRelease(JNIEnv *env, jclass clazz) {
    setCmdCallback(env, nullptr);
}

/*************FFmpeg cmd end************/
