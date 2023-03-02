//
// Created by weiyusong on 2023/3/1.
//

#ifndef FFMPEG_SAMPLE_AUDIODECODER_H
#define FFMPEG_SAMPLE_AUDIODECODER_H

#include "BaseDecoder.h"

#ifdef __cplusplus
extern "C" { //ffmpeg用c语言的方式编译
#endif

#include "libswresample/swresample.h"

#ifdef __cplusplus
}
#endif

class AudioDecoder : public BaseDecoder {

public:

    AudioDecoder(AVFormatContext *ftx, int streamIndex);

    ~AudioDecoder();

    bool init() override;

    void decode(AVPacket *packet) override;

    void flush() override;

    void setFrameAvailableListener(

    std::function<
    void(int8_t
    *pcmBuffer,
    int bufferSize
    )> listener);

private:

    int resample(AVFrame *avFrame);

    const int64_t OUT_CHANNEL = AV_CH_LAYOUT_STEREO;

    const AVSampleFormat OUT_FORMAT = AV_SAMPLE_FMT_S16;

    const int OUT_SAMPLE_RATE = 44100;

    AVCodec *mAudioCodec = nullptr;

    SwrContext *mSwrContext = nullptr;

    int8_t *mAudioBuffer = nullptr;

    std::function<
    void(int8_t
    *pcmBuffer,
    int bufferSize
    )>
    mFrameAvailableListener;

};


#endif //FFMPEG_SAMPLE_AUDIODECODER_H
