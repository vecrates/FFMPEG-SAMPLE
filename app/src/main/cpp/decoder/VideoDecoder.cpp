//
// Created by vecrates on 2022/11/29.
//

#include "VideoDecoder.h"
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/frame.h"
#include "libavutil/imgutils.h"

#ifdef __cplusplus
}
#endif

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"VideoDecoder",__VA_ARGS__)

VideoDecoder::VideoDecoder(AVFormatContext *ftx, int streamIndex)
        : BaseDecoder(ftx, streamIndex) {

}

VideoDecoder::~VideoDecoder() {
    if (mAvFrame != nullptr) {
        av_frame_free(&mAvFrame);
        mAvFrame = nullptr;
    }
    if (mAvCodecContext != nullptr) {
        avcodec_close(mAvCodecContext);
        avcodec_free_context(&mAvCodecContext);
        mAvCodecContext = nullptr;
    }
    mFrameAvailableListener = nullptr;
    mVideoCodec = nullptr;
}

bool VideoDecoder::init() {
    AVStream *stream = mAvFormatContext->streams[getStreamIndex()];
    if (stream == nullptr) {
        LOGE("not found stream: %d", getStreamIndex());
        return false;
    }

    AVCodecParameters *codecPar = stream->codecpar;
    mWidth = codecPar->width;
    mHeight = codecPar->height;

    AVDictionaryEntry *entry
            = av_dict_get(stream->metadata, "rotate", nullptr, AV_DICT_MATCH_CASE);
    if (entry != nullptr) {
        mRotate = std::atoi(entry->value);
    }

    //找到对应的解码器
    mVideoCodec = avcodec_find_decoder(codecPar->codec_id);
    if (mVideoCodec == nullptr) {
        LOGE("not found codec: %d", codecPar->codec_id);
        return false;
    }

    //创建CodecContext
    mAvCodecContext = avcodec_alloc_context3(mVideoCodec);
    //将码流参数复制到CodecContext
    avcodec_parameters_to_context(mAvCodecContext, codecPar);

    //打开解码器
    int ret = avcodec_open2(mAvCodecContext, mVideoCodec, nullptr);
    if (ret < 0) {
        LOGE("#codec open failed: %d", ret);
        return false;
    }

    //创建AvFrame，存储解码后的帧
    mAvFrame = av_frame_alloc();

    return true;
}

void VideoDecoder::decode(AVPacket *packet) {
    int ret = avcodec_send_packet(mAvCodecContext, packet);
    if (ret != 0 && ret != AVERROR(EAGAIN)) {
        LOGE("#avcodec_send_packet, error=%d", ret);
        return;
    }

    ret = avcodec_receive_frame(mAvCodecContext, mAvFrame);
    if (ret != 0) {
        LOGE("#avcodec_receive_frame, error=%d", ret);
        av_frame_unref(mAvFrame);
        return;
    }

    this->mCurrentTimestamp = ptsToUs(mAvFrame->pts);

    if (mFrameAvailableListener != nullptr) {
        mFrameAvailableListener(mAvFrame);
    }

    av_frame_unref(mAvFrame);

}

void VideoDecoder::flush() {
    BaseDecoder::flush();
}

void VideoDecoder::setFrameAvailableListener(std::function<void(AVFrame *)> listener) {
    this->mFrameAvailableListener = std::move(listener);
}

int VideoDecoder::getWidth() {
    return mWidth;
}

int VideoDecoder::getHeight() {
    return mHeight;
}
