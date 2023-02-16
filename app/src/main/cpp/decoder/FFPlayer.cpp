//
// Created by vecrates on 2022/12/10.
//

#include "FFPlayer.h"

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "libswscale/swscale.h"

#ifdef __cplusplus
}
#endif

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"FFPlayer",__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"FFPlayer",__VA_ARGS__)

FFPlayer::FFPlayer() {
    locker = new Locker();
}

FFPlayer::~FFPlayer() {
    reset();

    delete locker; //fixme 线程安全？
}

bool FFPlayer::prepare(const std::string &file) {

    LOGE("prepare:");

    if (mState != STATE::IDLE) {
        LOGE("prepare: illegal state %d", mState);
        return false;
    }

    mAvFormatContext = avformat_alloc_context();

    int ret = avformat_open_input(&mAvFormatContext, file.c_str(),
                                  nullptr, nullptr);
    if (ret != 0) {
        LOGE("prepare: avformat_open_input() failed, "
             "ret=%d error=%s file=%s", ret, av_err2str(ret), file.c_str());
        return false;
    }

    ret = avformat_find_stream_info(mAvFormatContext, nullptr);
    if (ret < 0) {
        LOGE("prepare: avformat_find_stream_info() failed, "
             "ret=%d error=%s file=%s", ret, av_err2str(ret), file.c_str());
        return false;
    }

    int videoStreamIndex = -1;
    int audioStreamIndex = -1;
    for (int i = 0; i < mAvFormatContext->nb_streams; ++i) {
        if (mAvFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
        } else if (mAvFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
        }
    }

    if (videoStreamIndex < 0) {
        LOGE("prepare: there is no video stream for this video, "
             "file=%s", file.c_str());
        return false;
    }

    mPreparing = true;

    mVideoDecoder = new VideoDecoder(mAvFormatContext, videoStreamIndex);
    mVideoDecoder->init();

    if (audioStreamIndex != -1) {
        //todo audio decoder
    }

    mVideoPacketQueue = new AvPacketQueue(60);
    mAudioPacketQueue = new AvPacketQueue(60);

    //std::thread 调用类的成员函数需要传递类的一个对象作为参数：
    mVideoDecodeThread = new std::thread(&FFPlayer::videoDecodeLoop, this);
    /*此函数必须在线程创建时立即调用，且调用此函数会使其不能被join
    没有执行join或detach的线程在程序结束时会引发异常*/

    updatePlayerState(STATE::PREPARED);

    return false;
}

void FFPlayer::setNativeWindow(ANativeWindow *nativeWindow, int width, int height) {
    if (mState == STATE::IDLE) {
        LOGE("setNativeWindow: illegal state %d", mState);
        return;
    }
    if (mVideoDecoder == nullptr) {
        LOGE("setNativeWindow: illegal state, decoder is null");
        return;
    }
    releaseNativeWindow();
    this->mNativeWindow = nativeWindow;
    ANativeWindow_setBuffersGeometry(mNativeWindow, width, height, WINDOW_FORMAT_RGBA_8888);
    mVideoDecoder->updateSws(width, height);
}

void FFPlayer::releaseNativeWindow() {
    ANativeWindow *lastNativeWindow = this->mNativeWindow;
    this->mNativeWindow = nullptr;
    if (lastNativeWindow != nullptr) {
        ANativeWindow_release(lastNativeWindow);
    }
}

bool FFPlayer::reset() {
    LOGE("reset:");
    if (mState == STATE::PLAYING) {
        stop();
    }
    mPreparing = false;
    LOGE("====%d", mPreparing);
    locker->notify();
    if (mVideoDecodeThread != nullptr) {
        mVideoDecodeThread->join();  //todo ANR风险
        mVideoDecodeThread = nullptr;
    }
    if (mAvFormatContext != nullptr) {
        avformat_close_input(&mAvFormatContext);
        avformat_free_context(mAvFormatContext);
        mAvFormatContext = nullptr;
    }
    if (mVideoDecoder != nullptr) {
        delete mVideoDecoder;
        mVideoDecoder = nullptr;
    }
    updatePlayerState(STATE::IDLE);
    return true;
}

void FFPlayer::start() {
    LOGE("start:");
    if (mState != STATE::PREPARED) {
        LOGE("start: illegal state %d", mState);
        return;
    }
    mRunning = true;
    mPacketReadThread = new std::thread(&FFPlayer::packetReadLoop, this);
    updatePlayerState(STATE::PLAYING);
}

void FFPlayer::stop() {
    LOGE("stop:");
    if (mState != STATE::PLAYING) {
        LOGE("stop: illegal state %d", mState);
        return;
    }
    mRunning = false;
    locker->notify();
    if (mPacketReadThread != nullptr) {
        mPacketReadThread->join();
        mPacketReadThread = nullptr;
    }
}

//region video

void FFPlayer::videoDecodeLoop() {
    LOGE("#videoDecodeLoop start");

    if (mVideoDecoder == nullptr) {
        LOGE("videoDecodeLoop: video decoder is null");
        return;
    }

    mVideoDecoder->setFrameAvailableListener([this](AVFrame *frame) {
        onVideoFrameAvailable(frame);
    });

    while (mPreparing) {

        if (mVideoPacketQueue == nullptr) {
            LOGE("exception: video queue is null");
            break;
        }

        LOGE("-----videoDecodeLoop mPreparing=%d", mPreparing);

        LOGE("mVideoPacketQueue pop, %d", mVideoPacketQueue->size());
        locker->lock();
        AVPacket *packet = mVideoPacketQueue->pop();
        if (packet == nullptr && mPreparing) {
            locker->waitWithoutLock(-1);
        }
        locker->unlock();

        LOGE("-----videoDecodeLoop>>>>> mPreparing=%d", mPreparing);

        if (!mPreparing) {
            if (packet != nullptr) {
                av_packet_free(&packet);
                av_freep(&packet);
            }
            break;
        }

        if (packet == nullptr) {
            continue;
        }

        mVideoDecoder->decode(packet);

        av_packet_free(&packet);
        av_freep(&packet);
    }

    LOGE("#videoDecodeLoop end");
}

void FFPlayer::onVideoFrameAvailable(AVFrame *avFrame) {
    LOGE("=====onVideoFrameAvailable");

    if (mNativeWindow == nullptr) {
        return;
    }

    LOGE("=====onVideoFrameAvailable>>>: %d %d", avFrame->height,
         ANativeWindow_getHeight(mNativeWindow));

    // lock native window buffer
    ANativeWindow_lock(mNativeWindow, &mNativeWindowBuffer, nullptr);

    //锁定当前 Window ，获取屏幕缓冲区 Buffer 的指针
    auto *dst = static_cast<uint8_t *>(mNativeWindowBuffer.bits);
    int dstStride = mNativeWindowBuffer.stride * 4;
    auto *src = (uint8_t * )(avFrame->data[0]);
    int srcStride = avFrame->linesize[0];
    // 由于window的stride和帧的stride不同,因此需要逐行复制
    int height = ANativeWindow_getHeight(mNativeWindow);
    for (int h = 0; h < height; h++) {
        memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
    }

    //解锁当前 Window ，渲染缓冲区数据
    ANativeWindow_unlockAndPost(mNativeWindow);
}

//endregion video

//region audio

void FFPlayer::audioDecodeLoop() {

}

//endregion

void FFPlayer::packetReadLoop() {
    LOGE("packetReadLoop: start");
    while (mRunning) {
        AVPacket *packet = av_packet_alloc();
        if (packet == nullptr) {
            LOGE("#av_packet_alloc failed");
            continue;
        }

        LOGE("-----packetReadLoop running=%d", mRunning);

        int ret = av_read_frame(mAvFormatContext, packet);
        if (ret != 0) {
            LOGE("#av_read_frame failed, ret=%d", ret);
            av_packet_free(&packet);
            av_freep(&packet);
            if (ret == AVERROR_EOF) {
                break;
            }
            continue;
        }

        if (mVideoDecoder == nullptr) {
            continue;
        }

        locker->lock();

        bool pushed = false;
        if (packet->stream_index == mVideoDecoder->getStreamIndex()
            && mVideoPacketQueue != nullptr) {
            LOGE("mVideoPacketQueue push, %d", mVideoPacketQueue->size());
            int trtTimes = 3;
            while (trtTimes-- > 0 && mRunning) {
                if (mVideoPacketQueue->push(packet)) {
                    pushed = true;
                    break;
                }
                locker->waitWithoutLock(10);
            }
        } else {
            //TODO audio
        }

        locker->unlock();
        if (pushed) {
            locker->notify();
        } else {
            LOGI("pack push failed");
            av_packet_free(&packet);
            av_freep(&packet);
        }

        locker->wait(10);

    }

    if (!mRunning) {
        locker->lock();
        mVideoPacketQueue->clear();
        locker->unlock();
    }

    LOGE("packetReadLoop: end");

}

void FFPlayer::updatePlayerState(STATE state) {
    LOGI("updatePlayerState, state=%d", state);
    this->mState = state;
}

