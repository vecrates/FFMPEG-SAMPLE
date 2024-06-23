//
// Created by vecrates on 2023/12/29.
//

#include "FFEncoder.h"
#include "libavcodec/jni.h"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"FFEncoder",__VA_ARGS__)

FFEncoder::~FFEncoder() {
    reset();
}

bool FFEncoder::init(JNIEnv *env, const string &srcFile, const string &outFile) {
    mFfPlayer = new FFPlayer();
    mFfPlayer->setFrameAvailableListener(std::bind(&FFEncoder::onFrameAvailable,
                                                   this,
                                                   placeholders::_1,
                                                   placeholders::_2));
    mFfPlayer->setDecodingFinishListener(std::bind(&FFEncoder::onDecodeFinish, this));
    bool ret = mFfPlayer->prepare(env, srcFile);
    if (!ret) {
        reset();
        return false;
    }

    mFfPlayer->setSync(false);
    mFfPlayer->setAudioResample(false);
    mDecoderInfo = mFfPlayer->getOutputInfo();
    if (!initOutput(outFile, mDecoderInfo)) {
        reset();
        return false;
    }

    return true;
}

bool FFEncoder::initOutput(const string &file, OutputInfo *outputInfo) {
    /*
     * 后面的三个参数，按着优先级，决定着*ctx的创建，如果oformat是null，则
     * format_name和filename起作用，如果format_name是空，则由filename起作用
     */
    int ret = avformat_alloc_output_context2(&mEncodeFormatContext, nullptr,
                                             nullptr, file.c_str());
    if (ret < 0) {
        LOGE("#avformat_alloc_output_context2: %s", av_err2str(ret));
        return false;
    }

    //AVFMT_NOFILE标志意味着没有AVIOContext, 编解码器将以其他的方式进行I/O操作
    if (!(mEncodeFormatContext->flags & AVFMT_NOFILE)) {
        ret = avio_open(&mEncodeFormatContext->pb, file.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("#avio_open: %s", av_err2str(ret));
            return false;
        }
    }

    if (!initOutputVideoStream(outputInfo)) {
        return false;
    }

    if (outputInfo->audioCodecId != AV_CODEC_ID_NONE
        && !initOutputAudioStream(outputInfo)) {
        return false;
    }

    av_dump_format(mEncodeFormatContext, 0, file.c_str(), 1);

    //如sps、pps等
    ret = avformat_write_header(mEncodeFormatContext, nullptr);
    if (ret < 0) {
        LOGE("#avformat_write_header: %s", av_err2str(ret));
        return false;
    }

    return true;
}

bool FFEncoder::initOutputVideoStream(OutputInfo *outputInfo) {
    mVideoStream = avformat_new_stream(mEncodeFormatContext, nullptr);
    if (mVideoStream == nullptr) {
        LOGE("#avformat_new_stream");
        return false;
    }
    mVideoStream->index = mEncodeFormatContext->nb_streams - 1;

    //mEncodeFormatContext->oformat->video_codec，默认的编码器
    //没用联合编译libx264时，创建成功但avcodec_open2：permission denied
    AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (encoder == nullptr) {
        LOGE("#avcodec_find_encoder");
        return false;
    }

    mVideoEncodeContext = avcodec_alloc_context3(encoder);
    if (mVideoEncodeContext == nullptr) {
        LOGE("#avcodec_alloc_context3");
        return false;
    }

    mVideoEncodeContext->width = outputInfo->width;
    mVideoEncodeContext->height = outputInfo->height;
    //mVideoEncodeContext->sample_aspect_ratio = outputInfo->sampleAspectRatio;
    //mVideoEncodeContext->bit_rate = 400000;
    //mVideoEncodeContext->gop_size = nFrameRate; // 每秒1个关键帧
    mVideoEncodeContext->time_base = AVRational{1, 25};
    mVideoEncodeContext->framerate = AVRational{25, 1};
    mVideoEncodeContext->pix_fmt = encoder->pix_fmts ?
                                   encoder->pix_fmts[0] : outputInfo->pixelFormat;

    if (mEncodeFormatContext->flags & AVFMT_GLOBALHEADER) {
        //告诉编码器它可以使用全局头文件
        mVideoEncodeContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    int ret = avcodec_open2(mVideoEncodeContext, encoder, nullptr);
    if (ret < 0) {
        LOGE("video#avcodec_open2: %s", av_err2str(ret));
        return false;
    }

    ret = avcodec_parameters_from_context(mVideoStream->codecpar, mVideoEncodeContext);
    if (ret < 0) {
        LOGE("#avcodec_parameters_from_context: %s", av_err2str(ret));
        return false;
    }

    mVideoStream->time_base = mVideoEncodeContext->time_base;

    return true;
}

bool FFEncoder::initOutputAudioStream(OutputInfo *outputInfo) {
    mAudioStream = avformat_new_stream(mEncodeFormatContext, nullptr);
    if (mAudioStream == nullptr) {
        LOGE("#avformat_new_stream");
        return false;
    }
    mAudioStream->index = mEncodeFormatContext->nb_streams - 1;

    AVCodec *encoder = avcodec_find_encoder(mEncodeFormatContext->oformat->audio_codec);
    if (encoder == nullptr) {
        LOGE("#avcodec_find_encoder");
        return false;
    }

    mAudioEncodeContext = avcodec_alloc_context3(encoder);
    if (mAudioEncodeContext == nullptr) {
        LOGE("#avcodec_alloc_context3");
        return false;
    }

    mAudioEncodeContext->sample_fmt = encoder->sample_fmts[0];
    mAudioEncodeContext->sample_rate = outputInfo->sampleRate; //实测采样率降低有杂音
    mAudioEncodeContext->channel_layout = AV_CH_LAYOUT_STEREO;
    mAudioEncodeContext->channels = av_get_channel_layout_nb_channels(
            mAudioEncodeContext->channel_layout);
    mAudioEncodeContext->time_base = AVRational{1, mAudioEncodeContext->sample_rate};
    mAudioEncodeContext->bit_rate = 128000;

    if (mEncodeFormatContext->flags & AVFMT_GLOBALHEADER) {
        //告诉编码器它可以使用全局头文件
        mAudioEncodeContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    int ret = avcodec_open2(mAudioEncodeContext, encoder, nullptr);
    if (ret < 0) {
        LOGE("audio#avcodec_open2: %s", av_err2str(ret));
        return false;
    }

    ret = avcodec_parameters_from_context(mAudioStream->codecpar, mAudioEncodeContext);
    if (ret < 0) {
        LOGE("#avcodec_parameters_from_context: %s", av_err2str(ret));
        return false;
    }

    mAudioStream->time_base = mAudioEncodeContext->time_base;

    mAudioResampleContext = swr_alloc();
    swr_alloc_set_opts(mAudioResampleContext,
                       mAudioEncodeContext->channel_layout,
                       mAudioEncodeContext->sample_fmt,
                       mAudioEncodeContext->sample_rate,
                       outputInfo->channelLayout,
                       outputInfo->sampleFormat,
                       outputInfo->sampleRate,
                       0, nullptr);
    //swr_init(mSwrContext)

    mResampleFrame = av_frame_alloc();
    mResampleFrame->sample_rate = mAudioEncodeContext->sample_rate;
    mResampleFrame->format = mAudioEncodeContext->sample_fmt;
    mResampleFrame->channel_layout = mAudioEncodeContext->channel_layout;
    av_frame_get_buffer(mResampleFrame, 0);

    return true;
}

void FFEncoder::reset() {
    closeEncode();
    if (mFfPlayer != nullptr) {
        mFfPlayer->stop();
        mFfPlayer->reset();
        delete mFfPlayer;
        mFfPlayer = nullptr;
    }
}

void FFEncoder::start() {
    if (mFfPlayer == nullptr || mFfPlayer->isPlaying()) {
        LOGE("#start: illegal state");
        return;
    }
    mFfPlayer->start();
}

void FFEncoder::stop() {
    if (mFfPlayer != nullptr) {
        mFfPlayer->stop();
    }
}

void FFEncoder::onFrameAvailable(AVFrame *avFrame, AVMediaType mediaType) {
    if (mediaType == AVMEDIA_TYPE_AUDIO) {
        AVFrame *encodeFrame = nullptr;
        if (avFrame != nullptr) {
            int ret = swr_convert_frame(mAudioResampleContext, mResampleFrame, avFrame);
            if (ret < 0) {
                LOGE("#swr_convert_frame: %s", av_err2str(ret));
                return;
            }
            mResampleFrame->pts = avFrame->pts;
            encodeFrame = mResampleFrame;
        }
        int ret = avcodec_send_frame(mAudioEncodeContext, encodeFrame);
        if (ret < 0) {
            LOGE("#avcodec_send_frame: %s", av_err2str(ret));
            return;
        }

        AVPacket *avPacket = av_packet_alloc();
        while (true) {
            ret = avcodec_receive_packet(mAudioEncodeContext, avPacket);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    av_interleaved_write_frame(mEncodeFormatContext, nullptr);
                }
                LOGE("audio#avcodec_receive_packet: %s", av_err2str(ret));
                break;
            }
            avPacket->stream_index = mAudioStream->index;
            AVRational timebase = mDecoderInfo->audioTimebase;
            av_packet_rescale_ts(avPacket, timebase, mAudioStream->time_base);
            LOGE(">>>write audio: %f<<<", avPacket->pts * av_q2d(mAudioStream->time_base));
            av_interleaved_write_frame(mEncodeFormatContext, avPacket);
        }
        av_packet_free(&avPacket);
    } else if (mediaType == AVMEDIA_TYPE_VIDEO) {
        int ret = avcodec_send_frame(mVideoEncodeContext, avFrame);
        if (ret < 0) {
            LOGE("#avcodec_send_frame: %s", av_err2str(ret));
            return;
        }
        AVPacket *avPacket = av_packet_alloc();
        while (true) {
            ret = avcodec_receive_packet(mVideoEncodeContext, avPacket);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    av_interleaved_write_frame(mEncodeFormatContext, nullptr);
                }
                LOGE("video#avcodec_receive_packet: %s", av_err2str(ret));
                break;
            }
            avPacket->stream_index = mVideoStream->index;
            AVRational timebase = mDecoderInfo->videoTimebase;
            av_packet_rescale_ts(avPacket, timebase, mVideoStream->time_base);
            LOGE(">>>write video: %f<<<", avPacket->pts * av_q2d(mVideoStream->time_base));
            av_interleaved_write_frame(mEncodeFormatContext, avPacket);
        }
        av_packet_free(&avPacket);
    }
}

void FFEncoder::onDecodeFinish() {
    //EOF
    onFrameAvailable(nullptr, AVMEDIA_TYPE_VIDEO);
    onFrameAvailable(nullptr, AVMEDIA_TYPE_AUDIO);
    //写输出文件尾
    LOGE("!!!write trailer");
    av_write_trailer(mEncodeFormatContext);

    reset();
}

void FFEncoder::closeEncode() {
    if (mEncodeFormatContext != nullptr) {
        if (mEncodeFormatContext->pb != nullptr) {
            avio_closep(&mEncodeFormatContext->pb);
            mEncodeFormatContext->pb = nullptr;
        }
        avformat_free_context(mEncodeFormatContext);
        mEncodeFormatContext = nullptr;
    }
    if (mVideoEncodeContext != nullptr) {
        avcodec_free_context(&mVideoEncodeContext);
        mVideoEncodeContext = nullptr;
    }
    if (mAudioEncodeContext != nullptr) {
        avcodec_free_context(&mAudioEncodeContext);
        mAudioEncodeContext = nullptr;
    }
    if (mAudioResampleContext != nullptr) {
        swr_free(&mAudioResampleContext);
        mAudioResampleContext = nullptr;
    }
    if (mResampleFrame != nullptr) {
        av_frame_free(&mResampleFrame);
        mResampleFrame = nullptr;
    }
    if (mDecoderInfo != nullptr) {
        delete mDecoderInfo;
        mDecoderInfo = nullptr;
    }
    mVideoStream = nullptr;
    mAudioStream = nullptr;
}