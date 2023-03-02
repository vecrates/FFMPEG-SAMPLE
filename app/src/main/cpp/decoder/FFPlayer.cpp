//
// Created by vecrates on 2022/12/10.
//

#include "FFPlayer.h"

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "../util/TimeUtil.h"

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
    mVideoQueueLocker = new Locker();
    mVideoSyncLocker = new Locker();
    mAudioQueueLocker = new Locker();
    mAudioSyncLocker = new Locker();
}

FFPlayer::~FFPlayer() {
    reset();

    delete mVideoQueueLocker; //fixme 线程安全？
    delete mVideoSyncLocker;
    delete mAudioQueueLocker;
    delete mAudioSyncLocker;
}

bool FFPlayer::prepare(JNIEnv *env, const std::string &file) {

    LOGE("prepare:");

    if (mState != STATE::IDLE) {
        LOGE("prepare: illegal state %d", mState);
        return false;
    }

    env->GetJavaVM(&mJvm);

    mAvFormatContext = avformat_alloc_context();

    /*LOGE("=============file format===============");
    av_dump_format(mAvFormatContext, 0, file.c_str(), 0);
    LOGE("=============file format===============");*/

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
        mAudioDecoder = new AudioDecoder(mAvFormatContext, audioStreamIndex);
        mAudioDecoder->init();
    }

    mVideoPacketQueue = new AvPacketQueue(60);
    mAudioPacketQueue = new AvPacketQueue(60);

    //std::thread 调用类的成员函数需要传递类的一个对象作为参数：
    mVideoDecodeThread = new std::thread(&FFPlayer::videoDecodeLoop, this);

    if (mAudioDecoder != nullptr) {
        mAudioDecodeThread = new std::thread(&FFPlayer::audioDecodeLoop, this);
    }

    /*attach函数必须在线程创建时立即调用，且调用此函数会使其不能被join
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
    notifyLockers();
    if (mVideoDecodeThread != nullptr) {
        mVideoDecodeThread->join();  //todo ANR风险
        mVideoDecodeThread = nullptr;
    }
    if (mAudioDecodeThread != nullptr) {
        mAudioDecodeThread->join();  //todo ANR风险
        mAudioDecodeThread = nullptr;
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
    if (mAudioDecoder != nullptr) {
        delete mAudioDecoder;
        mAudioDecoder = nullptr;
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
    notifyLockers();
    if (mPacketReadThread != nullptr) {
        mPacketReadThread->join();
        mPacketReadThread = nullptr;
    }
}

void FFPlayer::notifyLockers() {
    if (mVideoSyncLocker != nullptr) {
        mVideoSyncLocker->notify();
    }
    if (mVideoQueueLocker != nullptr) {
        mVideoQueueLocker->notify();
    }
    if (mAudioSyncLocker != nullptr) {
        mAudioSyncLocker->notify();
    }
    if (mAudioQueueLocker != nullptr) {
        mAudioQueueLocker->notify();
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

    long startSyncTimestamp = -1;
    long decodeStartTimestamp = 0;

    while (mPreparing) {

        if (mVideoPacketQueue == nullptr) {
            LOGE("exception: video queue is null");
            break;
        }

        LOGE("mVideoPacketQueue pop, %d", mVideoPacketQueue->size());
        mVideoQueueLocker->lock();
        AVPacket *packet = mVideoPacketQueue->pop();
        if (packet == nullptr && mPreparing) {
            mVideoQueueLocker->waitWithoutLock(-1);
        }
        mVideoQueueLocker->unlock();

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

        if (startSyncTimestamp < 0) {
            startSyncTimestamp = TimeUtil::timestampMicroSec();;
        }
        long escapedTimeUs = TimeUtil::timestampMicroSec() - startSyncTimestamp;
        long decodedTimeUs = mVideoDecoder->getCurrentTimestamp() - decodeStartTimestamp;
        long sleepMs = (decodedTimeUs - escapedTimeUs) / 1000;
        LOGE("#videoDecodeLoop slp=%ld", sleepMs);
        if (sleepMs > 0) {
            mVideoSyncLocker->wait(sleepMs);
        }

    }

    if (mJvm != nullptr) {
        mJvm->DetachCurrentThread();
    }

    LOGE("#videoDecodeLoop end");
}

void FFPlayer::onVideoFrameAvailable(JNIEnv *env, AVFrame *avFrame) {
    LOGE("=====onVideoFrameAvailable %d w=%d h=%d ",
         avFrame->format, avFrame->width, avFrame->height);
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

        if (mJniContext.jniListener != nullptr && mJniContext.videoFrameAvailable != nullptr) {
            env->CallVoidMethod(mJniContext.jniListener, mJniContext.videoFrameAvailable,
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
    LOGE("#audioDecodeLoop start");

    if (mAudioDecoder == nullptr) {
        LOGE("#audioDecodeLoop: audio decoder is null");
        return;
    }

    JNIEnv *env = nullptr;
    if (mJvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE("#audioDecodeLoop: attach thread failed");
        return;
    }

    mAudioDecoder->setFrameAvailableListener([this, env](int8_t *pcmBuffer, int bufferSize) {
        onAudioFrameAvailable(env, pcmBuffer, bufferSize);
    });

    long startSyncTimestamp = -1;
    long decodeStartTimestamp = 0;

    while (mPreparing) {

        if (mAudioPacketQueue == nullptr) {
            LOGE("exception: audio queue is null");
            break;
        }

        mAudioQueueLocker->lock();
        AVPacket *packet = mAudioPacketQueue->pop();
        if (packet == nullptr && mPreparing) {
            mAudioQueueLocker->waitWithoutLock(-1);
        }
        mAudioQueueLocker->unlock();

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

        mAudioDecoder->decode(packet);

        /*if (startSyncTimestamp < 0) {
            startSyncTimestamp = TimeUtil::timestampMicroSec();;
        }
        long escapedTimeUs = TimeUtil::timestampMicroSec() - startSyncTimestamp;
        long decodedTimeUs = mAudioDecoder->getCurrentTimestamp() - decodeStartTimestamp;
        long sleepMs = (decodedTimeUs - escapedTimeUs) / 1000;
        LOGE("#audioDecodeLoop slp=%ld", sleepMs);
        if (sleepMs > 0) {
            mAudioSyncLocker->wait(sleepMs);
        }*/

    }

    if (mJvm != nullptr) {
        mJvm->DetachCurrentThread();
    }

    LOGE("#audioDecodeLoop end");
}

void FFPlayer::onAudioFrameAvailable(JNIEnv *env, int8_t *pcmBuffer, int bufferSize) {
    LOGE("#onAudioFrameAvailable: size=%d", bufferSize);

    jbyteArray pcmBytes = env->NewByteArray(bufferSize);
    env->SetByteArrayRegion(pcmBytes, 0, bufferSize, pcmBuffer);

    if (mJniContext.jniListener != nullptr && mJniContext.audioFrameAvailable != nullptr) {
        env->CallVoidMethod(mJniContext.jniListener, mJniContext.audioFrameAvailable, pcmBytes);
    }

    env->DeleteLocalRef(pcmBytes);

}

//endregion

void FFPlayer::packetReadLoop() {
    LOGE("#packetReadLoop: start");

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

        bool pushed = false;
        if (packet->stream_index == mVideoDecoder->getStreamIndex()
            && mVideoPacketQueue != nullptr) {
            LOGE("mVideoPacketQueue push, %d", mVideoPacketQueue->size());
            mVideoQueueLocker->lock();
            int tryTimes = 3;
            while (tryTimes-- > 0 && mRunning) {
                if (mVideoPacketQueue->push(packet)) {
                    pushed = true;
                    break;
                }
                mVideoQueueLocker->waitWithoutLock(10);
            }
            mVideoQueueLocker->unlock();
            if (pushed) {
                mVideoQueueLocker->notify();
            }
        } else if (mAudioPacketQueue != nullptr
                   && mAudioDecoder != nullptr
                   && packet->stream_index == mAudioDecoder->getStreamIndex()) {
            LOGE("mAudioPacketQueue push, %d", mAudioPacketQueue->size());
            mAudioQueueLocker->lock();
            int tryTimes = 3;
            while (tryTimes-- > 0 && mRunning) {
                if (mAudioPacketQueue->push(packet)) {
                    pushed = true;
                    break;
                }
                mAudioQueueLocker->waitWithoutLock(10);
            }
            mAudioQueueLocker->unlock();
            if (pushed) {
                mAudioQueueLocker->notify();
            }
        } else {
            LOGE("unprocessed packet: %d", packet->stream_index);
            continue;
        }

        if (!pushed) {
            LOGI("packet push failed");
            av_packet_free(&packet);
            av_freep(&packet);
        }

    }

    if (!mRunning) {

        mVideoQueueLocker->lock();
        mVideoPacketQueue->clear();
        mVideoQueueLocker->unlock();

        mAudioQueueLocker->lock();
        mAudioPacketQueue->clear();
        mAudioQueueLocker->unlock();

    }

    LOGE("#packetReadLoop: end");

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

    mJniContext.jniListener = env->NewGlobalRef(jObj);

    jclass clazz = env->GetObjectClass(jObj);

    mJniContext.videoFrameAvailable = env->GetMethodID(clazz,
                                                       "onVideoFrameAvailable", "([B[B[B)V");

    mJniContext.audioFrameAvailable = env->GetMethodID(clazz,
                                                       "onAudioFrameAvailable", "([B)V");
}

void FFPlayer::resetJniListenContext(JNIEnv *env) {
    if (mJniContext.jniListener != nullptr) {
        jobject jniListener = mJniContext.jniListener;
        mJniContext.jniListener = nullptr;
        env->DeleteGlobalRef(jniListener);
    }
    mJniContext.videoFrameAvailable = nullptr;
    mJniContext.audioFrameAvailable = nullptr;
}

int *FFPlayer::getVideoSize() {
    if (mVideoDecoder == nullptr) {
        return new int[]{0, 0};
    }
    return new int[]{mVideoDecoder->getWidth(), mVideoDecoder->getHeight()};
}


//endregion listener

