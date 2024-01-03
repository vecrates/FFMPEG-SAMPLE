//
// Created by vecrates on 2023/12/29.
//

#ifndef FFMPEG_SAMPLE_FFENCODER_H
#define FFMPEG_SAMPLE_FFENCODER_H

#include <string>
#include "../decoder/FFPlayer.h"
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"

#ifdef __cplusplus
}
#endif


using namespace std;

class FFEncoder {

private:

    FFPlayer *mFfPlayer = nullptr;

    OutputInfo *mDecoderInfo = nullptr;

    AVFormatContext *mEncodeFormatContext = nullptr;

    AVCodecContext *mVideoEncodeContext = nullptr;

    AVCodecContext *mAudioEncodeContext = nullptr;

    AVStream *mVideoStream = nullptr;

    AVStream *mAudioStream = nullptr;

    SwrContext *mAudioResampleContext = nullptr;

    AVFrame *mResampleFrame = nullptr;

    void onFrameAvailable(AVFrame *avFrame, AVMediaType mediaType);

    void onDecodeFinish();

    bool initOutput(const string &file, OutputInfo *outputInfo);

    bool initOutputVideoStream(OutputInfo *outputInfo);

    bool initOutputAudioStream(OutputInfo *outputInfo);

    void closeEncode();

public:

    ~FFEncoder();

    bool init(JNIEnv *env, const string &srcFile, const string &outFile);

    void start();

    void stop();

    void reset();


};


#endif //FFMPEG_SAMPLE_FFENCODER_H
