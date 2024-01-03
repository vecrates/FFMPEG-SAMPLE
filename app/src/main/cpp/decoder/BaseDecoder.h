//
// Created by vecrates on 2022/12/1.
//

#ifndef FFMPEG_SAMPLE_BASEDECODER_H
#define FFMPEG_SAMPLE_BASEDECODER_H

#include <functional>

#ifdef __cplusplus
extern "C" { //ffmpeg用c语言的方式编译
#endif

#include "libavformat/avformat.h"

#ifdef __cplusplus
}
#endif

class BaseDecoder {

public:
    //构造函数不能为virtual，否则违背构造函数的含义
    BaseDecoder(AVFormatContext *ftx, int streamIndex);

    //应该是virtual，否则delete时调用的是基类析构，子类内存泄露
    virtual ~BaseDecoder();

    virtual bool init();

    virtual void decode(AVPacket *packet);

    /*
     * 清空缓存帧
     */
    virtual void flush();

    virtual void seekTo(long us);

    int getStreamIndex();

    long getCurrentTimestamp();

    virtual AVRational getTimebase() final;

    long ptsToUs(int64_t pts);

    long usToPts(long us);

    virtual void setFrameAvailableListener(std::function<void(AVFrame *)> listener) final;

protected:

    AVFormatContext *mAvFormatContext = nullptr;

    AVCodecContext *mDecodeContext = nullptr;

    AVFrame *mAvFrame = nullptr;

    long mCurrentTimestamp = 0; //us

    long mDuration; //us

    AVRational mTimeBase{}; //stream timebase

    std::function<void(AVFrame *)> mFrameAvailableListener = nullptr;

private:

    int mStreamIndex = -1;

};


#endif //FFMPEG_SAMPLE_BASEDECODER_H
