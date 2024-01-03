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

struct OutputInfo {
    int width;
    int height;
    AVRational sampleAspectRatio;
    AVCodecID videoCodecId = AV_CODEC_ID_NONE;
    AVPixelFormat pixelFormat;
    int64_t bitRate;
    AVRational videoTimebase;
    //
    AVCodecID audioCodecId = AV_CODEC_ID_NONE;
    int sampleRate;
    AVSampleFormat sampleFormat;
    uint64_t channelLayout;
    AVRational audioTimebase;
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

    void setFrameAvailableListener(std::function<void(AVFrame *, AVMediaType)> listener);

    void setDecodingFinishListener(std::function<void()> listener);

    /**
     * 进入 IDLE 状态
     */
    bool reset();

    void start();

    void stop();

    int *getVideoSize();

    OutputInfo *getOutputInfo();

    bool isPlaying();

    void setSync(bool sync);

    void setAudioResample(bool resample);

private:

    void videoDecodeLoop();

    void onVideoFrameAvailable(JNIEnv *env, AVFrame *avFrame);

    void onAudioFrameAvailable(JNIEnv *env, AVFrame *avFrame);

    void audioDecodeLoop();

    void packetReadLoop();

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

    JniListenContext mJniListenContext{};

    std::function<void(AVFrame *, AVMediaType)> mFrameAvailableListener = nullptr;

    std::function<void()> mDecodeFinishListener = nullptr;

    bool mPreparing = false;

    bool mRunning = false; //标识是否真正读取、解码

    bool mSync = true; //同步音视频速度

};


#endif //FFMPEG_SAMPLE_FFPLAYER_H
