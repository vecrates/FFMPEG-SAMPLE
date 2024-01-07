//
// Created by vecrates on 2023/12/13.
//

#include "AudioMixer.h"
#include <android/log.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"AudioMixer",__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"AudioMixer",__VA_ARGS__)

using namespace std;

AudioMixer::AudioMixer() = default;

AudioMixer::~AudioMixer() {
    reset();
}

bool AudioMixer::addAudio(const string &filePath) {
    if (mInitialized) {
        LOGE("mixer is initialized");
        return false;
    }

    AVFormatContext *avFormatContext = avformat_alloc_context();
    int ret = avformat_open_input(&avFormatContext, filePath.c_str(),
                                  nullptr, nullptr);
    if (ret != 0) {
        avformat_free_context(avFormatContext);
        LOGE("file open failed：%s", filePath.c_str());
        return false;
    }

    //读取媒体信息
    ret = avformat_find_stream_info(avFormatContext, nullptr);
    if (ret != 0) {
        avformat_free_context(avFormatContext);
        LOGE("failed to call #avformat_find_stream_info：error=%s %s", av_err2str(ret),
             filePath.c_str());
        return false;
    }

    AVStream *audioStream = nullptr;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = avFormatContext->streams[i];
            break;
        }
    }

    if (audioStream == nullptr) {
        avformat_free_context(avFormatContext);
        LOGE("no audio stream：%s", filePath.c_str());
        return false;
    }

    AVCodec *avCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (avCodec == nullptr) {
        avformat_free_context(avFormatContext);
        LOGE("#avcodec_find_decoder: %s", filePath.c_str());
        return false;
    }

    AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
    if (avFormatContext == nullptr) {
        avformat_free_context(avFormatContext);
        LOGE("#avcodec_alloc_context3: %s", filePath.c_str());
        return false;
    }

    avcodec_parameters_to_context(avCodecContext, audioStream->codecpar);

    if (avCodecContext->channel_layout == 0
        && avCodecContext->channels == 0) {
        LOGE("the audio's channels is 0: %s", filePath.c_str());
        avformat_free_context(avFormatContext);
        avcodec_free_context(&avCodecContext);
        return false;
    }

    //部分音频格式不填充channel_layout字段，如 wma
    if (avCodecContext->channel_layout == 0) {
        //通道数->通道格式
        avCodecContext->channel_layout = av_get_default_channel_layout(
                avCodecContext->channels);
    }

    ret = avcodec_open2(avCodecContext, avCodec, nullptr);
    if (ret < 0) {
        avcodec_free_context(&avCodecContext);
        avformat_free_context(avFormatContext);
        LOGE("#avcodec_open2: error=%s %s", av_err2str(ret), filePath.c_str());
        return false;
    }

    AudioInfo audioInfo;
    audioInfo.id = audioIdGen++;
    audioInfo.filePath = filePath;
    mAudioInfos.push_back(audioInfo);

    auto *audioContext = new AudioContext();
    audioContext->id = audioInfo.id;
    audioContext->avStream = audioStream;
    audioContext->avCodecContext = avCodecContext;
    audioContext->avFormatContext = avFormatContext;
    audioContext->fifoReadFrame = newAudioFrame(avCodecContext, false);
    audioContext->emptyFrame = newAudioFrame(avCodecContext, true);
    mAudioContexts.insert({audioInfo.id, audioContext});

    /*//stream.timeBase和codecContext.timeBase大概率不同
    uint64_t seekT = usToPts(audioContext->avStream->time_base, 60000000);
    int r = av_seek_frame(audioContext->avFormatContext,
                          audioContext->avStream->index,
                          seekT, AVSEEK_FLAG_BACKWARD);*/

    return true;
}

AVFrame *AudioMixer::newAudioFrame(AVCodecContext *avCodecContext, bool fill) {
    //与从解码中获取帧不同，这里需要根据帧的信息为帧分配存储数据的内存块，
    //从fifo中读取帧数据时，使用此frame对象
    //alloc仅分配了AVFrame自身的内存，data、buffer引用的内存未分配和设置
    AVFrame *newFrame = av_frame_alloc();
    newFrame->format = avCodecContext->sample_fmt;
    newFrame->sample_rate = avCodecContext->sample_rate;
    newFrame->channel_layout = avCodecContext->channel_layout;
    newFrame->nb_samples = (int) (avCodecContext->sample_rate * 0.1); //读取0.1s的量
    /*if (avCodecContext->frame_size <= 0) {
        newFrame->nb_samples = (int) (avCodecContext->sample_rate * 0.1); //读取0.1s的量
    } else {
        newFrame->nb_samples = avCodecContext->frame_size; //读取一帧的量
    }*/
    //为frame分配存储帧数据的内存，另外还有 av_image_alloc、av_image_fill_arrays
    av_frame_get_buffer(newFrame, 0);
    if (fill) {
        //填充空数据
        for (auto &data: newFrame->data) {
            if (data == nullptr) {
                continue;
            }
            memset(data, 0, newFrame->linesize[0] * sizeof(uint8_t));
        }
    }
    return newFrame;
}

bool AudioMixer::init() {
    if (mInitialized) {
        LOGE("mixer has been mInitialized");
        return false;
    }

    if (mAudioInfos.empty()) {
        LOGE("the audios is empty");
        return false;
    }

    mFilterGraph = avfilter_graph_alloc();
    if (mFilterGraph == nullptr) {
        LOGE("failed to call #avfilter_graph_alloc");
        return false;
    }

    //输入
    string volumeDesc;
    string amixInDesc;
    {
        AVFilterInOut *lastSrcOut = nullptr;
        for (const auto &it: mAudioInfos) {
            string inputName = "in" + std::to_string(it.id);

            auto contextIt = mAudioContexts.find(it.id);
            if (contextIt == mAudioContexts.end()) {
                //unreachable
                reset();
                return false;
            }

            AVCodecContext *audioCodecContext = contextIt->second->avCodecContext;

            const AVFilter *srcBufferFilter = avfilter_get_by_name("abuffer");
            AVFilterContext *srcFilterContext = nullptr;
            char args[128] = {'\0'};
            char channelLayoutHex[16] = {'\0'};
            sprintf(channelLayoutHex, "0X%lX", audioCodecContext->channel_layout);
            snprintf(args, sizeof(args),
                     "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
                     audioCodecContext->time_base.num,
                     audioCodecContext->time_base.den,
                     audioCodecContext->sample_rate,
                     av_get_sample_fmt_name(audioCodecContext->sample_fmt),
                     channelLayoutHex);
            LOGE("src=%s args=%s", inputName.c_str(), args);
            int ret = avfilter_graph_create_filter(&srcFilterContext,
                                                   srcBufferFilter,
                                                   inputName.c_str(), //name随意
                                                   args,
                                                   nullptr,
                                                   mFilterGraph);
            if (ret < 0) {
                LOGE("#avfilter_graph_create_filter, error=%s %s", av_err2str(ret),
                     it.filePath.c_str());
                reset();
                return false;
            }

            //音量调整为保持不变，1/n*n
            string volumeOutputName = "_" + inputName;
            volumeDesc.append("[" + inputName + "]");
            volumeDesc.append("volume=" + std::to_string(mAudioContexts.size()));
            volumeDesc.append("[" + volumeOutputName + "],");
            //混音输入
            amixInDesc.append("[" + volumeOutputName + "]");

            //引脚
            AVFilterInOut *srcOut = avfilter_inout_alloc();
            srcOut->name = av_strdup(inputName.c_str());
            srcOut->filter_ctx = srcFilterContext;
            srcOut->pad_idx = 0;
            srcOut->next = nullptr;
            if (mSrcOuts == nullptr) {
                mSrcOuts = srcOut;
            }
            if (lastSrcOut == nullptr) {
                lastSrcOut = srcOut;
            } else {
                lastSrcOut->next = srcOut;
                lastSrcOut = srcOut;
            }

            AudioContext *audioContext = contextIt->second;
            audioContext->avFilterContext = srcFilterContext;
            //audioContext->avFilterInOut = srcOut;
            audioContext->isPlanar = av_sample_fmt_is_planar(
                    audioContext->avCodecContext->sample_fmt);
            audioContext->avAudioFifo = av_audio_fifo_alloc(
                    audioContext->avCodecContext->sample_fmt,
                    audioContext->avCodecContext->channels,
                    1);
        }
    }

    //输出
    string outDesc;
    {
        string outputName = "out";
        outDesc = "[" + outputName + "]";
        const AVFilter *sinkBufferFilter = avfilter_get_by_name("abuffersink");
        AVFilterContext *sinkFilterContext = nullptr;
        int ret = avfilter_graph_create_filter(&sinkFilterContext,
                                               sinkBufferFilter,
                                               outputName.c_str(), //name随意
                                               nullptr,
                                               nullptr,
                                               mFilterGraph);
        if (ret < 0) {
            reset();
            LOGE("#avfilter_graph_create_filter: %s", av_err2str(ret));
            return false;
        }

        mSinkFilterContext = sinkFilterContext;

        int sample_rates[] = {44100, -1};
        AVSampleFormat sinkSampleFormats[] = {AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE};
        int64_t channelLayouts[] = {AV_CH_LAYOUT_STEREO, -1};
        //设置输出格式
        ret = av_opt_set_int_list(sinkFilterContext, "sample_rates", sample_rates, -1,
                                  AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            reset();
            LOGE("sample_rates set failed, error=%s", av_err2str(ret));
            return false;
        }

        ret = av_opt_set_int_list(sinkFilterContext, "sample_fmts", sinkSampleFormats, -1,
                                  AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            reset();
            LOGE("sample_fmts set failed, error=%s", av_err2str(ret));
            return false;
        }
        ret = av_opt_set_int_list(sinkFilterContext, "channel_layouts", channelLayouts, -1,
                                  AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            reset();
            LOGE("channel_layouts set failed, error=%s", av_err2str(ret));
            return false;
        }

        AVFilterInOut *sinkIn = avfilter_inout_alloc();
        sinkIn->name = av_strdup(outputName.c_str());
        sinkIn->filter_ctx = sinkFilterContext;
        sinkIn->pad_idx = 0;
        sinkIn->next = nullptr;
        mSinkIn = sinkIn;

    }

    //amix
    string amixDesc = volumeDesc;
    amixDesc.append(amixInDesc);
    amixDesc.append("amix=inputs=");
    amixDesc.append(std::to_string(mAudioInfos.size()));
    amixDesc.append(outDesc);

    LOGE("filters descr: %s", amixDesc.c_str());

    //执行后，inputs和outputs都被置null
    int ret = avfilter_graph_parse_ptr(mFilterGraph, amixDesc.c_str(),
                                       &mSinkIn, &mSrcOuts, nullptr);
    if (ret < 0) {
        LOGE("#avfilter_graph_parse_ptr: %s", av_err2str(ret));
        reset();
        return false;
    }

    ret = avfilter_graph_config(mFilterGraph, nullptr);
    if (ret < 0) {
        LOGE("#avfilter_graph_config: %s", av_err2str(ret));
        reset();
        return false;
    }

    mInitialized = true;

    LOGI("AudioMixer is initialized");

    return true;
}

void AudioMixer::reset() {
    if (mSrcOuts != nullptr) {
        //只需传第一个，会按next清理清理整个链表
        avfilter_inout_free(&mSrcOuts);
        mSrcOuts = nullptr;
    }
    if (mSinkIn != nullptr) {
        avfilter_inout_free(&mSinkIn);
        mSinkIn = nullptr;
    }
    for (const auto &it: mAudioContexts) {
        AudioContext *audioContext = it.second;
        if (audioContext->avCodecContext != nullptr) {
            avcodec_close(audioContext->avCodecContext);
            avcodec_free_context(&audioContext->avCodecContext);
            audioContext->avCodecContext = nullptr;
        }
        if (audioContext->avFormatContext != nullptr) {
            avformat_close_input(&audioContext->avFormatContext);
            avformat_free_context(audioContext->avFormatContext);
            audioContext->avFormatContext = nullptr;
        }
        if (audioContext->avFilterContext != nullptr) {
            avfilter_free(audioContext->avFilterContext);
            audioContext->avFilterContext = nullptr;
        }
        if (audioContext->avAudioFifo != nullptr) {
            av_audio_fifo_free(audioContext->avAudioFifo);
            audioContext->avAudioFifo = nullptr;
        }
        if (audioContext->fifoReadFrame != nullptr) {
            av_frame_free(&audioContext->fifoReadFrame);
            audioContext->fifoReadFrame = nullptr;
        }
        if (audioContext->emptyFrame != nullptr) {
            av_frame_free(&audioContext->emptyFrame);
            audioContext->emptyFrame = nullptr;
        }
        delete audioContext;
    }
    mAudioContexts.clear();
    if (mSinkFilterContext != nullptr) {
        avfilter_free(mSinkFilterContext);
        mSinkFilterContext = nullptr;
    }
    if (mFilterGraph != nullptr) {
        avfilter_graph_free(&mFilterGraph);
        mFilterGraph = nullptr;
    }
    mAudioInfos.clear();
    audioIdGen = 0;
    mInitialized = false;
}

AudioPCM *AudioMixer::readFrame() {
    AVFrame *outFrame = av_frame_alloc();
    while (true) {
        addFrame2Buffer();
        int ret = av_buffersink_get_frame(mSinkFilterContext, outFrame);
        if (AVERROR(ret) == EAGAIN) {
            //输入数据不够，继续添加
            continue;
        }
        if (ret < 0) {
            av_frame_free(&outFrame);
            auto *audioPcm = new AudioPCM();
            audioPcm->ptr = new uint8_t[]{0};
            audioPcm->size = 1;
            LOGE("#av_buffersink_get_frame: %s", av_err2str(ret));
            return audioPcm;
        }
        break;
    }

    auto *audioPcm = new AudioPCM();
    audioPcm->ptr = outFrame->data[0];
    // nb_samples * channels * bytes_per_sample
    audioPcm->size = outFrame->nb_samples * 2 * 2;

    outFrame->data[0] = nullptr;
    outFrame->buf[0] = nullptr;
    av_frame_free(&outFrame);

    return audioPcm;
}

void AudioMixer::addFrame2Buffer() {
    for (auto &it: mAudioContexts) {
        int id = it.first;
        AudioContext *audioContext = it.second;

        writeFifo(audioContext);

        int fifoSamples = av_audio_fifo_size(audioContext->avAudioFifo);
        int readSamples = min(fifoSamples, audioContext->fifoReadFrame->nb_samples);
        if (readSamples <= 0) {
            LOGE("#av_audio_fifo_size: fifo_samples=%d id=%d", fifoSamples, id);
            continue;
        }

        int ret = av_audio_fifo_read(audioContext->avAudioFifo,
                                     (void **) audioContext->fifoReadFrame->data,
                                     readSamples);

        if (ret <= 0) {
            LOGE("#av_audio_fifo_read: error=%s id=%d", av_err2str(ret), id);
            continue;
        }

        ret = av_buffersrc_add_frame_flags(audioContext->avFilterContext,
                                           audioContext->fifoReadFrame,
                                           AV_BUFFERSRC_FLAG_KEEP_REF);

        if (ret < 0) {
            LOGE("#av_buffersrc_add_frame_flags: %s", av_err2str(ret));
        }

    }
}

void AudioMixer::writeFifo(AudioContext *audioContext) {
    //队列中数据量至少不小于一帧
    int minSamples = audioContext->fifoReadFrame->nb_samples;
    while (av_audio_fifo_size(audioContext->avAudioFifo) < minSamples) {
        if (audioContext->isEof || !writeAudioFrame2Fiwrifo(audioContext)) {
            writeEmptyFrame2Fifo(audioContext);
        }
    }
}

bool AudioMixer::writeAudioFrame2Fiwrifo(AudioContext *audioContext) {
    AVPacket *avPacket = av_packet_alloc();
    if (avPacket == nullptr) {
        LOGE("#av_packet_alloc");
        return false;
    }

    int ret = av_read_frame(audioContext->avFormatContext, avPacket);
    if (ret != 0) {
        if (ret == AVERROR_EOF) {
            audioContext->isEof = true;
        }
        av_packet_free(&avPacket);
        LOGE("#av_read_frame: %s %d", av_err2str(ret), audioContext->id);
        return false;
    }

    ret = avcodec_send_packet(audioContext->avCodecContext, avPacket);
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        av_packet_free(&avPacket);
        LOGE("#avcodec_send_packet: %s", av_err2str(ret));
        return false;
    }

    AVFrame *avFrame = av_frame_alloc();
    if (avFrame == nullptr) {
        av_packet_free(&avPacket);
        LOGE("#av_frame_alloc");
        return false;
    }

    ret = avcodec_receive_frame(audioContext->avCodecContext, avFrame);
    if (ret != 0) {
        av_packet_free(&avPacket);
        av_frame_free(&avFrame);
        LOGE("#avcodec_receive_frame");
        return false;
    }

    /*audioContext->lastDecodeTimeUs = ptsToUs(audioContext->avCodecContext->time_base,
                                             avFrame->pts);*/

    if (audioContext->isPlanar
        && audioContext->avCodecContext->channels > 1
        && avFrame->data[1] == nullptr) {
        av_packet_free(&avPacket);
        av_frame_free(&avFrame);
        LOGE("format is planer, but channel 1 is null");
        return false;
    }

    ret = av_audio_fifo_write(audioContext->avAudioFifo,
                              (void **) avFrame->data,
                              avFrame->nb_samples);
    if (ret < 0) {
        LOGE("#av_audio_fifo_write: %s", av_err2str(ret));
    }

    av_packet_free(&avPacket);
    av_frame_free(&avFrame);

    return ret >= 0;
}

void AudioMixer::writeEmptyFrame2Fifo(AudioContext *audioContext) {
    int ret = av_audio_fifo_write(audioContext->avAudioFifo,
                                  (void **) audioContext->emptyFrame->data,
                                  audioContext->emptyFrame->nb_samples);
    if (ret < 0) {
        LOGE("writeEmptyFrame2Fifo #av_audio_fifo_write: %s", av_err2str(ret));
    }
}

long AudioMixer::ptsToUs(AVRational timeBase, int64_t pts) {
    return pts * av_q2d(timeBase) * 1000000L;
}

int64_t AudioMixer::usToPts(AVRational timeBase, long us) {
    double s = (double) us / 1000000;
    return (uint64_t) round(s / av_q2d(timeBase));
//    int64_t startTime = s * AV_TIME_BASE;
//    int64_t target_time = av_rescale_q(startTime, AV_TIME_BASE_Q, timeBase);
//    return target_time;
}
