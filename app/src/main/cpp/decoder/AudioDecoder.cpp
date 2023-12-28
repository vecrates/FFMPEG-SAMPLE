//
// Created by weiyusong on 2023/3/1.
//

#include "AudioDecoder.h"
#include "../util/TimeUtil.h"
#include <android/log.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"AudioDecoder",__VA_ARGS__)

AudioDecoder::AudioDecoder(AVFormatContext *ftx, int streamIndex)
        : BaseDecoder(ftx, streamIndex) {

}

AudioDecoder::~AudioDecoder() {
    if (mAvFrame != nullptr) {
        av_frame_free(&mAvFrame);
        mAvFrame = nullptr;
    }
    if (mAvCodecContext != nullptr) {
        avcodec_close(mAvCodecContext);
        avcodec_free_context(&mAvCodecContext);
        mAvCodecContext = nullptr;
    }
    if (mSwrContext != nullptr) {
        swr_free(&mSwrContext);
        mSwrContext = nullptr;
    }
    if (mAudioBuffer != nullptr) {
        free(mAudioBuffer);
        mAudioBuffer = nullptr;
    }
    mFrameAvailableListener = nullptr;
    mAudioCodec = nullptr;
}

bool AudioDecoder::init() {
    AVStream *avStream = mAvFormatContext->streams[getStreamIndex()];
    if (avStream == nullptr) {
        LOGE("not found stream: %d", getStreamIndex());
        return false;
    }

    AVCodecParameters *codecPar = avStream->codecpar;
    mAudioCodec = avcodec_find_decoder(codecPar->codec_id);
    if (mAudioCodec == nullptr) {
        LOGE("not found codec: %d", codecPar->codec_id);
        return false;
    }

    mAvCodecContext = avcodec_alloc_context3(mAudioCodec);
    //将码流参数复制到CodecContext
    avcodec_parameters_to_context(mAvCodecContext, codecPar);

    int ret = avcodec_open2(mAvCodecContext, mAudioCodec, nullptr);
    if (ret < 0) {
        LOGE("#codec open failed: %d", ret);
        return false;
    }

    //重采样
    mSwrContext = swr_alloc();
    swr_alloc_set_opts(mSwrContext,
                       OUT_CHANNEL, //输出声道
                       OUT_FORMAT, //输出数据大小及格式
                       OUT_SAMPLE_RATE, //输出采样率
                       mAvCodecContext->channel_layout,
                       mAvCodecContext->sample_fmt,
                       mAvCodecContext->sample_rate,
                       0, nullptr);

    ret = swr_init(mSwrContext);
    if (ret < 0) {
        LOGE("#SwrContext init failed: %d", ret);
        return false;
    }

    mAvFrame = av_frame_alloc();

    return true;
}

void AudioDecoder::decode(AVPacket *packet) {
    int ret = avcodec_send_packet(mAvCodecContext, packet);
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        LOGE("#avcodec_send_packet, error=%d", ret);
        return;
    }

    ret = avcodec_receive_frame(mAvCodecContext, mAvFrame);
    if (ret != 0) {
        LOGE("#avcodec_receive_frame, error=%d", ret);
        av_frame_unref(mAvFrame);
        return;
    }

    int bufferSize = resample(mAvFrame);

    this->mCurrentTimestamp = ptsToUs(mAvFrame->pts);

    if (mFrameAvailableListener != nullptr) {
        mFrameAvailableListener(mAudioBuffer, bufferSize);
    }

    av_frame_unref(mAvFrame);

}

void AudioDecoder::flush() {
    BaseDecoder::flush(); //todo
}

int AudioDecoder::resample(AVFrame *avFrame) {
    //计算重采样后样本个数
    //保持重采样后 dst_nb_samples / dest_sample = src_nb_sample / src_sample_rate
    int outNbSamples = (int) av_rescale_rnd(avFrame->nb_samples, OUT_SAMPLE_RATE,
                                            avFrame->sample_rate, AV_ROUND_UP);
    //第二种写法：+256让空间足够
    //int outNbSamples = (int64_t) avFrame->nb_samples * OUT_SAMPLE_RATE / avFrame->sample_rate + 256;

    //根据通道数、样本数、数据格式，返回数据大小
    int outNbChannels = av_get_channel_layout_nb_channels(OUT_CHANNEL);
    int outBufferSize = av_samples_get_buffer_size(nullptr,
                                                   outNbChannels,
                                                   outNbSamples,
                                                   OUT_FORMAT,
                                                   1);
    if (outBufferSize < 0) {
        LOGE("#av_samples_get_buffer_size, error=%d", outBufferSize);
        return 0;
    }

    if (mAudioBuffer == nullptr) {
        mAudioBuffer = (int8_t *) (av_malloc(outBufferSize));
    }

    int finalOutNbSamples = swr_convert(mSwrContext,
                                        reinterpret_cast<uint8_t **>(&mAudioBuffer), // 输出数据的指针
                                        outNbSamples, //输出单通道的样本数量，不是字节数。单通道的样本数量。
                                        (const uint8_t **) avFrame->data, //输⼊数据的指针
                                        avFrame->nb_samples //输⼊的单通道的样本数量
    );
    if (finalOutNbSamples < 0) {
        LOGE("#swr_convert, error=%d", finalOutNbSamples);
        return 0;
    }

    int perSampleBytes = av_get_bytes_per_sample(OUT_FORMAT);

    //buffer size
    return finalOutNbSamples * outNbChannels * perSampleBytes;
}

void AudioDecoder::setFrameAvailableListener(std::function<void(int8_t *, int)> listener) {
    this->mFrameAvailableListener = std::move(listener);
}

