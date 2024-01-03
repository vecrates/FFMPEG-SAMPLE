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
    if (mDecodeContext != nullptr) {
        avcodec_close(mDecodeContext);
        avcodec_free_context(&mDecodeContext);
        mDecodeContext = nullptr;
    }
}

bool VideoDecoder::init() {
    AVStream *stream = mAvFormatContext->streams[getStreamIndex()];
    if (stream == nullptr) {
        LOGE("not found stream: %d", getStreamIndex());
        return false;
    }

    AVCodecParameters *codecPar = stream->codecpar;

    /*AVDictionaryEntry *entry
            = av_dict_get(stream->metadata, "rotate", nullptr, AV_DICT_MATCH_CASE);
    if (entry != nullptr) {
        mRotate = std::atoi(entry->value);
    }*/

    //找到对应的解码器
    AVCodec *videoCodec = avcodec_find_decoder(codecPar->codec_id);
    if (videoCodec == nullptr) {
        LOGE("not found codec: %d", codecPar->codec_id);
        return false;
    }

    //创建CodecContext
    mDecodeContext = avcodec_alloc_context3(videoCodec);
    //将码流参数复制到CodecContext
    avcodec_parameters_to_context(mDecodeContext, codecPar);

    //打开解码器
    int ret = avcodec_open2(mDecodeContext, videoCodec, nullptr);
    if (ret < 0) {
        LOGE("#codec open failed: %d", ret);
        return false;
    }

    //创建AvFrame，存储解码后的帧
    mAvFrame = av_frame_alloc();

    return true;
}

void VideoDecoder::decode(AVPacket *packet) {
    int ret = avcodec_send_packet(mDecodeContext, packet);
    if (ret != 0 && ret != AVERROR(EAGAIN)) {
        LOGE("#avcodec_send_packet, error=%d", ret);
        return;
    }

    ret = avcodec_receive_frame(mDecodeContext, mAvFrame);
    if (ret != 0) {
        LOGE("#avcodec_receive_frame, error=%d", ret);
        av_frame_unref(mAvFrame);
        return;
    }

    this->mCurrentTimestamp = ptsToUs(mAvFrame->best_effort_timestamp);
    //this->mCurrentTimestamp = ptsToUs(mAvFrame->pts);

    if (mFrameAvailableListener != nullptr) {
        mFrameAvailableListener(mAvFrame);
    }

    av_frame_unref(mAvFrame);

}

void VideoDecoder::flush() {
    BaseDecoder::flush();
}

AVCodecID VideoDecoder::getCodecId() {
    return mDecodeContext->codec_id;
}

int VideoDecoder::getWidth() {
    return mDecodeContext != nullptr ? mDecodeContext->width : 0;
}

int VideoDecoder::getHeight() {
    return mDecodeContext != nullptr ? mDecodeContext->height : 0;
}

AVRational VideoDecoder::getAspectRatio() {
    return mDecodeContext != nullptr ? mDecodeContext->sample_aspect_ratio : AVRational{};
}

AVPixelFormat VideoDecoder::getPixelFormat() {
    return mDecodeContext != nullptr ? mDecodeContext->pix_fmt : AV_PIX_FMT_NONE;
}

int64_t VideoDecoder::getBitrate() {
    return mDecodeContext != nullptr ? mDecodeContext->bit_rate : 0;
}

void VideoDecoder::seekTo(long us) {
    if (mDecodeContext == nullptr) {
        return;
    }

    av_seek_frame(mAvFormatContext, getStreamIndex(),
                  usToPts(us), AVSEEK_FLAG_BACKWARD);
}
