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
    bool prepare(const std::string &file);

    void setNativeWindow(ANativeWindow *nativeWindow, int width, int height);

    void releaseNativeWindow();

    /**
     * 进入 IDLE 状态
     */
    bool reset();

    void start();

    void stop();

    void videoDecodeLoop();

    void onVideoFrameAvailable(AVFrame *avFrame);

    void audioDecodeLoop();

    void packetReadLoop();

private:

    void notifyThreads();

    void updatePlayerState(STATE state);

    Locker *locker = nullptr;

    AVFormatContext *mAvFormatContext = nullptr;

    VideoDecoder *mVideoDecoder = nullptr;

    std::thread *mVideoDecodeThread = nullptr;

    std::thread *mAudioDecodeThread = nullptr;

    std::thread *mPacketReadThread = nullptr;

    AvPacketQueue *mVideoPacketQueue = nullptr;

    AvPacketQueue *mAudioPacketQueue = nullptr;

    STATE mState = STATE::IDLE;

    ANativeWindow *mNativeWindow = nullptr;

    ANativeWindow_Buffer mNativeWindowBuffer;

    bool mPreparing = false;

    bool mRunning = false;

};


#endif //FFMPEG_SAMPLE_FFPLAYER_H
