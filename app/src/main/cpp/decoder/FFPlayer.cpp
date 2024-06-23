//
// Created by vecrates on 2022/12/10.
//

#include "FFPlayer.h"

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <utility>
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

    //mAvFormatContext = avformat_alloc_context();

    /*LOGE("=============file format===============");
    av_dump_format(mAvFormatContext, 0, file.c_str(), 0);
    LOGE("=============file format===============");*/

    int ret = avformat_open_input(&mAvFormatContext, file.c_str(),
                                  nullptr, nullptr);
    if (ret != 0) {
        LOGE("prepare: avformat_open_input() failed, "
             "error=%s file=%s", av_err2str(ret), file.c_str());
        return false;
    }

    ret = avformat_find_stream_info(mAvFormatContext, nullptr);
    if (ret < 0) {
        LOGE("prepare: avformat_find_stream_info() failed, "
             "error=%s file=%s", av_err2str(ret), file.c_str());
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

    return true;
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
    mJniListenContext.jniListener = nullptr;
    mJniListenContext.audioFrameAvailable = nullptr;
    mJniListenContext.videoFrameAvailable = nullptr;
    mFrameAvailableListener = nullptr;
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

void FFPlayer::packetReadLoop() {
    LOGE("#packetReadLoop: start");

    if (mVideoDecoder != nullptr) {
        mVideoDecoder->seekTo(0);
    }
    if (mAudioDecoder != nullptr) {
        mAudioDecoder->seekTo(0);
    }

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
            //LOGE("mVideoPacketQueue push, %d", mVideoPacketQueue->size());
            mVideoQueueLocker->lock();
            while (mRunning) {
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
            while (mRunning) {
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

    while (true) {
        if (!mVideoPacketQueue->isEmpty()) {
            mVideoQueueLocker->wait(-1);
        } else if (!mAudioPacketQueue->isEmpty()) {
            mAudioQueueLocker->wait(-1);
        } else {
            break;
        }
    }

    mRunning = false;
    updatePlayerState(STATE::PREPARED);

    if (mDecodeFinishListener != nullptr) {
        mDecodeFinishListener();
    }

    LOGE("#packetReadLoop: end");
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

        mVideoQueueLocker->lock();
        AVPacket *packet = mVideoPacketQueue->pop();
        //LOGE("mVideoPacketQueue pop, size=%d", mVideoPacketQueue->size());
        if (packet == nullptr && mPreparing) {
            mVideoQueueLocker->waitWithoutLock(-1);
        }
        mVideoQueueLocker->unlock();
        mVideoQueueLocker->notify();

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

        if (!mSync) {
            continue;
        }

        if (startSyncTimestamp < 0) {
            startSyncTimestamp = TimeUtil::timestampMicroSec();
        }
        long escapedTimeUs = TimeUtil::timestampMicroSec() - startSyncTimestamp;
        long decodedTimeUs = mVideoDecoder->getCurrentTimestamp() - decodeStartTimestamp;
        long sleepMs = (decodedTimeUs - escapedTimeUs) / 1000;
        LOGE("#videoDecodeLoop slp=%ld curTime=%ld", sleepMs, mVideoDecoder->getCurrentTimestamp());
        if (sleepMs > 0) {
            mVideoSyncLocker->wait(sleepMs);
        }

        //todo 丢帧，注意I帧

    }

    if (mJvm != nullptr) {
        mJvm->DetachCurrentThread();
    }

    LOGE("#videoDecodeLoop end");
}

void FFPlayer::onVideoFrameAvailable(JNIEnv *env, AVFrame *avFrame) {
    if (mFrameAvailableListener != nullptr) {
        mFrameAvailableListener(avFrame, AVMEDIA_TYPE_VIDEO);
        return;
    }

    LOGE("=====onVideoFrameAvailable %d w=%d h=%d lineSize=%d",
         avFrame->format, avFrame->width, avFrame->height,
         avFrame->linesize[0]);
    if (avFrame->format == AV_PIX_FMT_YUV420P) {

        int frameWidth = avFrame->linesize[0]; //内存对齐，不一定等于width
        int frameHeight = avFrame->height;

        //unsigned char=byte
        unsigned char *y = avFrame->data[0];
        unsigned char *u = avFrame->data[1];
        unsigned char *v = avFrame->data[2];

        //avFrame->colorspace; bt.601、bt709...

        int frameSize = frameWidth * frameHeight;

        jbyteArray yBytes = env->NewByteArray(frameSize);
        env->SetByteArrayRegion(yBytes, 0, frameSize, (jbyte *) y);

        jbyteArray uBytes = env->NewByteArray(frameSize / 4);
        env->SetByteArrayRegion(uBytes, 0, frameSize / 4, (jbyte *) u);

        jbyteArray vBytes = env->NewByteArray(frameSize / 4);
        env->SetByteArrayRegion(vBytes, 0, frameSize / 4, (jbyte *) v);

        if (mJniListenContext.jniListener != nullptr
            && mJniListenContext.videoFrameAvailable != nullptr) {
            env->CallVoidMethod(mJniListenContext.jniListener,
                                mJniListenContext.videoFrameAvailable,
                                yBytes, uBytes, vBytes, frameWidth, frameHeight);
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

    mAudioDecoder->setFrameAvailableListener([this, env](AVFrame *avFrame) {
        onAudioFrameAvailable(env, avFrame);
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

        av_packet_free(&packet);
        av_freep(&packet);

        if (!mSync) {
            continue;
        }

        if (startSyncTimestamp < 0) {
            startSyncTimestamp = TimeUtil::timestampMicroSec();;
        }
        long escapedTimeUs = TimeUtil::timestampMicroSec() - startSyncTimestamp;
        long decodedTimeUs = mAudioDecoder->getCurrentTimestamp() - decodeStartTimestamp;
        long sleepMs = (decodedTimeUs - escapedTimeUs) / 1000;
        LOGE("#audioDecodeLoop slp=%ld curTime=%ld esc=%ld", sleepMs,
             mAudioDecoder->getCurrentTimestamp(),
             escapedTimeUs);
        if (sleepMs > 0) {
            mAudioSyncLocker->wait(sleepMs);
        }

    }

    if (mJvm != nullptr) {
        mJvm->DetachCurrentThread();
    }

    LOGE("#audioDecodeLoop end");
}

void FFPlayer::onAudioFrameAvailable(JNIEnv *env, AVFrame *avFrame) {
    if (mFrameAvailableListener != nullptr) {
        mFrameAvailableListener(avFrame, AVMEDIA_TYPE_AUDIO);
        return;
    }

    int sampleBytes = av_get_bytes_per_sample(static_cast<AVSampleFormat>(avFrame->format));
    int frameSize = avFrame->nb_samples * avFrame->channels * sampleBytes;
    LOGE("#onAudioFrameAvailable: size=%d linesize=%d", frameSize, avFrame->linesize[0]);

    jbyteArray pcmBytes = env->NewByteArray(frameSize);
    env->SetByteArrayRegion(pcmBytes, 0, frameSize, (jbyte *) avFrame->data[0]);

    if (mJniListenContext.jniListener != nullptr
        && mJniListenContext.audioFrameAvailable != nullptr) {
        env->CallVoidMethod(mJniListenContext.jniListener,
                            mJniListenContext.audioFrameAvailable,
                            pcmBytes);
    }

    env->DeleteLocalRef(pcmBytes);

}

//endregion


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

    mJniListenContext.jniListener = env->NewGlobalRef(jObj);

    jclass clazz = env->GetObjectClass(jObj);

    mJniListenContext.videoFrameAvailable = env->GetMethodID(clazz,
                                                             "onVideoFrameAvailable",
                                                             "([B[B[BII)V");

    mJniListenContext.audioFrameAvailable = env->GetMethodID(clazz,
                                                             "onAudioFrameAvailable", "([B)V");
}

void FFPlayer::resetJniListenContext(JNIEnv *env) {
    if (mJniListenContext.jniListener != nullptr) {
        jobject jniListener = mJniListenContext.jniListener;
        mJniListenContext.jniListener = nullptr;
        env->DeleteGlobalRef(jniListener);
    }
    mJniListenContext.videoFrameAvailable = nullptr;
    mJniListenContext.audioFrameAvailable = nullptr;
}

void FFPlayer::setFrameAvailableListener(std::function<void(AVFrame *, AVMediaType)> listener) {
    this->mFrameAvailableListener = std::move(listener);
}

void FFPlayer::setDecodingFinishListener(std::function<void()> listener) {
    this->mDecodeFinishListener = std::move(listener);
}

//endregion listener


int *FFPlayer::getVideoSize() {
    if (mVideoDecoder == nullptr) {
        return new int[]{0, 0};
    }
    return new int[]{mVideoDecoder->getWidth(), mVideoDecoder->getHeight()};
}

OutputInfo *FFPlayer::getOutputInfo() {
    auto *outputInfo = new OutputInfo;
    if (mVideoDecoder == nullptr) {
        return outputInfo;
    }
    outputInfo->videoCodecId = mVideoDecoder->getCodecId();
    outputInfo->width = mVideoDecoder->getWidth();
    outputInfo->height = mVideoDecoder->getHeight();
    outputInfo->sampleAspectRatio = mVideoDecoder->getAspectRatio();
    outputInfo->pixelFormat = mVideoDecoder->getPixelFormat();
    outputInfo->bitRate = mVideoDecoder->getBitrate();
    outputInfo->videoTimebase = mVideoDecoder->getTimebase();
    if (mAudioDecoder != nullptr) {
        outputInfo->audioCodecId = mAudioDecoder->getCodecId();
        outputInfo->sampleRate = mAudioDecoder->getSampleRate();
        outputInfo->sampleFormat = mAudioDecoder->getSampleFormat();
        outputInfo->channelLayout = mAudioDecoder->getChannelLayout();
        outputInfo->audioTimebase = mAudioDecoder->getTimebase();
    }
    return outputInfo;
}


bool FFPlayer::isPlaying() {
    return mState == STATE::PLAYING;
}

void FFPlayer::setSync(bool sync) {
    FFPlayer::mSync = sync;
}

void FFPlayer::setAudioResample(bool resample) {
    if (mAudioDecoder != nullptr) {
        mAudioDecoder->setResample(resample);
    }
}

