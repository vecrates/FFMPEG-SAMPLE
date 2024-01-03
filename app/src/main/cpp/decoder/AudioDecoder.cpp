//
// Created by weiyusong on 2023/3/1.
//

#include "AudioDecoder.h"
#include "../util/TimeUtil.h"
#include <android/log.h>
#include <string>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"AudioDecoder",__VA_ARGS__)

AudioDecoder::AudioDecoder(AVFormatContext *ftx, int streamIndex)
        : BaseDecoder(ftx, streamIndex) {

}

AudioDecoder::~AudioDecoder() {
    if (mAvFrame != nullptr) {
        av_frame_free(&mAvFrame);
        mAvFrame = nullptr;
    }
    if (mResampleFrame != nullptr) {
        av_frame_free(&mResampleFrame);
        mResampleFrame = nullptr;
    }
    if (mDecodeContext != nullptr) {
        avcodec_close(mDecodeContext);
        avcodec_free_context(&mDecodeContext);
        mDecodeContext = nullptr;
    }
    if (mSwrContext != nullptr) {
        swr_free(&mSwrContext);
        mSwrContext = nullptr;
    }
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

    mDecodeContext = avcodec_alloc_context3(mAudioCodec);
    //将码流参数复制到CodecContext
    avcodec_parameters_to_context(mDecodeContext, codecPar);

    int ret = avcodec_open2(mDecodeContext, mAudioCodec, nullptr);
    if (ret < 0) {
        LOGE("#codec open failed: %d", ret);
        return false;
    }

    //重采样
    mSwrContext = swr_alloc();
    swr_alloc_set_opts(mSwrContext,
                       OUT_CHANNEL_LAYOUT, //输出声道
                       OUT_FORMAT, //输出数据大小及格式
                       OUT_SAMPLE_RATE, //输出采样率
                       mDecodeContext->channel_layout,
                       mDecodeContext->sample_fmt,
                       mDecodeContext->sample_rate,
                       0, nullptr);

    /*
    * 如果使用swr_convert_frame进行格式转换，则swr_init可以不用写
    * 如果使用swr_convert进行格式转换，则需要使用swr_init函数进行初始化
    */
    ret = swr_init(mSwrContext);
    if (ret < 0) {
        LOGE("#SwrContext init failed: %d", ret);
        return false;
    }

    mAvFrame = av_frame_alloc();

    return true;
}

void AudioDecoder::decode(AVPacket *packet) {
    int ret = avcodec_send_packet(mDecodeContext, packet);
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        LOGE("#avcodec_send_packet, error=%d", ret);
        return;
    }

    ret = avcodec_receive_frame(mDecodeContext, mAvFrame);
    if (ret != 0) {
        LOGE("#avcodec_receive_frame, error=%d", ret);
        av_frame_unref(mAvFrame);
        return;
    }

    if (isResample) {
        resample(mAvFrame);
    }

    this->mCurrentTimestamp = ptsToUs(mAvFrame->best_effort_timestamp);
    //this->mCurrentTimestamp = ptsToUs(mAvFrame->pts);

    if (mFrameAvailableListener != nullptr) {
        AVFrame *avFrame = isResample ? mResampleFrame : mAvFrame;
        mFrameAvailableListener(avFrame);
    }

    av_frame_unref(mAvFrame);

}

void AudioDecoder::flush() {
    BaseDecoder::flush(); //todo
}

int AudioDecoder::resample(AVFrame *avFrame) {
    if (mResampleFrame == nullptr) {
        mResampleFrame = av_frame_alloc();
        mResampleFrame->sample_rate = OUT_SAMPLE_RATE;
        mResampleFrame->format = OUT_FORMAT;
        mResampleFrame->channel_layout = OUT_CHANNEL_LAYOUT;
        /*mResampleFrame->nb_samples = (int) av_rescale_rnd(avFrame->nb_samples,
                                                          mResampleFrame->sample_rate,
                                                          avFrame->sample_rate,
                                                          AV_ROUND_UP);*/
        av_frame_get_buffer(mResampleFrame, 0);
    }

    int ret = swr_convert_frame(mSwrContext, mResampleFrame, avFrame);
    if (ret < 0) {
        LOGE("#swr_convert_frame, error=%s", av_err2str(ret));
        return 0;
    }

    return ret;
}

/*int AudioDecoder::resample(AVFrame *avFrame) {
    //计算重采样后样本个数
    //保持重采样后 dst_nb_samples / dest_sample_rate = src_nb_sample / src_sample_rate
    int outNbSamples = (int) av_rescale_rnd(avFrame->nb_samples, OUT_SAMPLE_RATE,
                                            avFrame->sample_rate, AV_ROUND_UP);
    //第二种写法：+256让空间足够
    //int outNbSamples = (int64_t) avFrame->nb_samples * OUT_SAMPLE_RATE / avFrame->sample_rate + 256;

    //根据通道数、样本数、数据格式，返回数据大小
    int outNbChannels = av_get_channel_layout_nb_channels(OUT_CHANNEL_LAYOUT);
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
}*/

AVCodecID AudioDecoder::getCodecId() {
    return mDecodeContext->codec_id;
}

int AudioDecoder::getSampleRate() {
    return mDecodeContext->sample_rate;
}

AVSampleFormat AudioDecoder::getSampleFormat() {
    return mDecodeContext->sample_fmt;
}

uint64_t AudioDecoder::getChannelLayout() {
    return mDecodeContext->channel_layout;
}

void AudioDecoder::setResample(bool resample) {
    isResample = resample;
}

void AudioDecoder::seekTo(long us) {
    if (mDecodeContext == nullptr) {
        return;
    }
    av_seek_frame(mAvFormatContext, getStreamIndex(),
                  usToPts(0), AVSEEK_FLAG_ANY);
}
