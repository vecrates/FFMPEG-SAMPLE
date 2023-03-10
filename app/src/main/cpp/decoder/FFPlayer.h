//
// Created by vecrates on 2022/12/10.
//

#ifndef FFMPEG_SAMPLE_FFPLAYER_H
#define FFMPEG_SAMPLE_FFPLAYER_H

#include <jni.h>
#include <string>
#include <thread>
#include "VideoDecoder.h"
#include "AvPacketQueue.h"
#include "../util/Locker.h"
#include "AudioDecoder.h"

struct JniListenContext {
    jobject jniListener = nullptr;
    jmethodID videoFrameAvailable = nullptr;
    jmethodID audioFrameAvailable = nullptr;
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

    void onAudioFrameAvailable(JNIEnv *env, int8_t *pcmBuffer, int bufferSize);

    void audioDecodeLoop();

    void packetReadLoop();

    int *getVideoSize();

private:

    void updatePlayerState(STATE state);

    void resetJniListenContext(JNIEnv *env);

    void notifyLockers();

    JavaVM *mJvm = nullptr;

    Locker *mVideoQueueLocker = nullptr;

    Locker *mVideoSyncLocker = nullptr;

    Locker *mAudioQueueLocker = nullptr;

    Locker *mAudioSyncLocker = nullptr;

    AVFormatContext *mAvFormatContext = nullptr;

    VideoDecoder *mVideoDecoder = nullptr;

    AudioDecoder *mAudioDecoder = nullptr;

    std::thread *mVideoDecodeThread = nullptr;

    std::thread *mAudioDecodeThread = nullptr;

    std::thread *mPacketReadThread = nullptr;

    AvPacketQueue *mVideoPacketQueue = nullptr;

    AvPacketQueue *mAudioPacketQueue = nullptr;

    STATE mState = STATE::IDLE;

    JniListenContext mJniContext{};

    bool mPreparing = false;

    bool mRunning = false;

};


#endif //FFMPEG_SAMPLE_FFPLAYER_H
