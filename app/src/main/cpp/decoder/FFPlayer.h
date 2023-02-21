//
// Created by vecrates on 2022/12/10.
//

#ifndef FFMPEG_SAMPLE_FFPLAYER_H
#define FFMPEG_SAMPLE_FFPLAYER_H

#include <jni.h>
#include <string>
#include <thread>
#include <android/native_window.h>
#include "VideoDecoder.h"
#include "AvPacketQueue.h"
#include "../util/Locker.h"

struct JniListenContext {
    jobject jniListener;
    jmethodID videoFrameAvailable;
    jmethodID audioFrameAvailable;
};

enum class STATE {
    IDLE,
    PREPARED,
    PLAYING,
};

class FFPlayer {

public:

    FFPlayer();

    ~FFPlayer();

    /**
     * 进入 PREPARED 状态
     * @param file
     * @return
     */
    bool prepare(JNIEnv *env, const std::string &file);

    void setJNIListenContext(JNIEnv *env, jobject jObj);

    /**
     * 进入 IDLE 状态
     */
    bool reset();

    void start();

    void stop();

    void videoDecodeLoop();

    void onVideoFrameAvailable(JNIEnv *env, AVFrame *avFrame);

    void audioDecodeLoop();

    void packetReadLoop();

private:

    void updatePlayerState(STATE state);

    void resetJniListenContext(JNIEnv *env);

    JavaVM *mJvm = nullptr;

    Locker *locker = nullptr;

    AVFormatContext *mAvFormatContext = nullptr;

    VideoDecoder *mVideoDecoder = nullptr;

    std::thread *mVideoDecodeThread = nullptr;

    std::thread *mAudioDecodeThread = nullptr;

    std::thread *mPacketReadThread = nullptr;

    AvPacketQueue *mVideoPacketQueue = nullptr;

    AvPacketQueue *mAudioPacketQueue = nullptr;

    STATE mState = STATE::IDLE;

    JniListenContext jniContext;

    bool mPreparing = false;

    bool mRunning = false;

};


#endif //FFMPEG_SAMPLE_FFPLAYER_H
