//
// Created by vecrates on 2022/11/29.
//

#ifndef FFMPEG_SAMPLE_VIDEODECODER_H
#define FFMPEG_SAMPLE_VIDEODECODER_H

#include "BaseDecoder.h"

#ifdef __cplusplus
extern "C" { //ffmpeg用c语言的方式编译
#endif

#include "libswscale/swscale.h"

#ifdef __cplusplus
}
#endif

class VideoDecoder : public BaseDecoder {

public:
    VideoDecoder(AVFormatContext *ftx, int streamIndex);

    ~VideoDecoder();

    bool init() override;

    void decode(AVPacket *packet) override;

    void flush() override;

    void seekTo(long us) override;

    AVCodecID getCodecId();

    int getWidth();

    int getHeight();

    AVRational getAspectRatio();

    AVPixelFormat getPixelFormat();

    int64_t getBitrate();

private:

};


#endif //FFMPEG_SAMPLE_VIDEODECODER_H
