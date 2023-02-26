//
// Created by vecrates on 2022/11/29.
//

#include "VideoDecoder.h"
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/frame.h"
#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"VideoDecoder",__VA_ARGS__)

VideoDecoder::VideoDecoder(AVFormatContext *ftx, int streamIndex)
        : BaseDecoder(ftx, streamIndex) {

}

VideoDecoder::~VideoDecoder() {
    release();
}

void VideoDecoder::release() {
    if (mAvFrame != nullptr) {
        av_frame_free(&mAvFrame);
        mAvFrame = nullptr;
    }
    if (mCodecContext != nullptr) {
        avcodec_close(mCodecContext);
        avcodec_free_context(&mCodecContext);
        mCodecContext = nullptr;
    }
    mFrameAvailableListener = nullptr;
    mVideoCodec = nullptr;
}

bool VideoDecoder::init() {
    AVStream *stream = mAvFormatContext->streams[getStreamIndex()];
    AVCodecParameters *codecPar = stream->codecpar;
    mWidth = codecPar->width;
    mHeight = codecPar->height;

    AVDictionaryEntry *entry
            = av_dict_get(stream->metadata, "rotate", nullptr, AV_DICT_MATCH_CASE);

    if (entry != nullptr) {
        LOGE("----->%s=%s", entry->key, entry->value);
    } else {
        LOGE("-----null");
    }

    //找到对应的解码器
    mVideoCodec = avcodec_find_decoder(codecPar->codec_id);
    if (mVideoCodec == nullptr) {
        LOGE("avcodec_find_decoder() invoke failed");
        return false;
    }

    //创建CodecContext
    mCodecContext = avcodec_alloc_context3(mVideoCodec);
    //将码流参数复制到CodecContext
    int ret = avcodec_parameters_to_context(mCodecContext, codecPar);
    if (ret < 0) {
        LOGE("#avcodec_parameters_to_context, result=%d", ret);
        return false;
    }

    //打开解码器
    ret = avcodec_open2(mCodecContext, mVideoCodec, nullptr);
    if (ret < 0) {
        LOGE("#avcodec_open2, result=%d", ret);
        return false;
    }

    //创建AvFrame，存储解码后的帧
    mAvFrame = av_frame_alloc();

    return true;
}

void VideoDecoder::decode(AVPacket *packet) {
    int ret = avcodec_send_packet(mCodecContext, packet);
    if (ret != 0 && ret != AVERROR(EAGAIN)) {
        LOGE("#avcodec_send_packet, error=%d", ret);
        return;
    }

    ret = avcodec_receive_frame(mCodecContext, mAvFrame);
    if (ret != 0) {
        LOGE("#avcodec_receive_frame, error=%d", ret);
        av_frame_unref(mAvFrame);
        return;
    }

    long frameTimeSec = mAvFrame->pts;
    this->mCurrentTimestamp = frameTimeSec * av_q2d(mTimeBase) * 1000;

    if (mFrameAvailableListener != nullptr) {
        mFrameAvailableListener(mAvFrame);
    }

    av_frame_unref(mAvFrame);

}

void VideoDecoder::flush() {
    BaseDecoder::flush();
}

int VideoDecoder::getWidth() {
    return mWidth;
}

int VideoDecoder::getHeight() {
    return mHeight;
}

long VideoDecoder::getCurrentTimestamp() {
    return mCurrentTimestamp;
}


