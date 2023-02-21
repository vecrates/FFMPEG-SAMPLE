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

bool FFPlayer::prepare(JNIEnv *env, const std::string &file) {

    LOGE("prepare:");

    if (mState != STATE::IDLE) {
        LOGE("prepare: illegal state %d", mState);
        return false;
    }

    env->GetJavaVM(&mJvm);

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

bool FFPlayer::reset() {
    LOGE("reset:");
    if (mState == STATE::PLAYING) {
        stop();
    }
    mPreparing = false;
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
    mJvm = nullptr;
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
        LOGE("#videoDecodeLoop: video decoder is null");
        return;
    }

    JNIEnv *env = nullptr;
    if (mJvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("#videoDecodeLoop: attach thread failed");
        return;
    }

    mVideoDecoder->setFrameAvailableListener([this, env](AVFrame *frame) {
        onVideoFrameAvailable(env, frame);
    });

    while (mPreparing) {

        if (mVideoPacketQueue == nullptr) {
            LOGE("exception: video queue is null");
            break;
        }

        LOGE("mVideoPacketQueue pop, %d", mVideoPacketQueue->size());
        locker->lock();
        AVPacket *packet = mVideoPacketQueue->pop();
        if (packet == nullptr && mPreparing) {
            locker->waitWithoutLock(-1);
        }
        locker->unlock();

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

    if (mJvm != nullptr) {
        mJvm->DetachCurrentThread();
    }

    LOGE("#videoDecodeLoop end");
}

void FFPlayer::onVideoFrameAvailable(JNIEnv *env, AVFrame *avFrame) {
    LOGE("=====onVideoFrameAvailable");
    if (avFrame->format == AV_PIX_FMT_YUV420P) {

        //unsigned char=byte
        unsigned char *y = avFrame->data[0];
        unsigned char *u = avFrame->data[1];
        unsigned char *v = avFrame->data[2];

        int frameSize = avFrame->width * avFrame->height;

        jbyteArray yBytes = env->NewByteArray(frameSize);
        env->SetByteArrayRegion(yBytes, 0, frameSize, (jbyte *) y);

        jbyteArray uBytes = env->NewByteArray(frameSize / 4);
        env->SetByteArrayRegion(uBytes, 0, frameSize / 4, (jbyte *) u);

        jbyteArray vBytes = env->NewByteArray(frameSize / 4);
        env->SetByteArrayRegion(vBytes, 0, frameSize / 4, (jbyte *) v);

        if (jniContext.jniListener != nullptr && jniContext.videoFrameAvailable != nullptr) {
            env->CallVoidMethod(jniContext.jniListener, jniContext.videoFrameAvailable,
                                yBytes, uBytes, vBytes);
        }

        env->DeleteLocalRef(yBytes);
        env->DeleteLocalRef(uBytes);
        env->DeleteLocalRef(vBytes);

        return;
    }

    LOGE("#onVideoFrameAvailable, unsupported format: %d", avFrame->format);
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


//region listener

void FFPlayer::setJNIListenContext(JNIEnv *env, jobject jObj) {
    if (jObj == nullptr) {
        resetJniListenContext(env);
        return;
    }

    jniContext.jniListener = env->NewGlobalRef(jObj);

    jclass clazz = env->GetObjectClass(jObj);

    jniContext.videoFrameAvailable = env->GetMethodID(clazz,
                                                      "onVideoFrameAvailable", "([B[B[B)V");

    jniContext.audioFrameAvailable = env->GetMethodID(clazz,
                                                      "onAudioFrameAvailable", "([B)V");
}

void FFPlayer::resetJniListenContext(JNIEnv *env) {
    if (jniContext.jniListener != nullptr) {
        jobject jniListener = jniContext.jniListener;
        jniContext.jniListener = nullptr;
        env->DeleteGlobalRef(jniListener);
    }
    jniContext.videoFrameAvailable = nullptr;
    jniContext.audioFrameAvailable = nullptr;
}

//endregion listener

