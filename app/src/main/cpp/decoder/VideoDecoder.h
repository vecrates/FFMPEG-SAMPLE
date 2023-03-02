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

    void setFrameAvailableListener(std::function<void(AVFrame *)>);

    int getWidth();

    int getHeight();

private:

    AVCodec *mVideoCodec = nullptr;

    std::function<void(AVFrame *)> mFrameAvailableListener = nullptr;

    int mWidth = 0;

    int mHeight = 0;

    int mRotate = 0;

    AVRational mFrameRate;

};


#endif //FFMPEG_SAMPLE_VIDEODECODER_H
