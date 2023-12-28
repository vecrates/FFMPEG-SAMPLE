//
// Created by vecrates on 2023/12/13.
//

#ifndef FFMPEG_SAMPLE_AUDIOMIXER_H
#define FFMPEG_SAMPLE_AUDIOMIXER_H

#include <string>
#include <vector>
#include <map>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/audio_fifo.h"

#ifdef __cplusplus
}
#endif

using namespace std;

struct AudioInfo {
    int id;
    string filePath;
};

struct AudioPCM {
    uint8_t *ptr = nullptr;
    int size = 0; //bytes

    ~AudioPCM() {
        if (ptr != nullptr) {
            delete ptr;
            ptr = nullptr;
        }
        size = 0;
    }
};

struct AudioContext {
    int id = -1;
    AVStream *avStream = nullptr;
    AVCodecContext *avCodecContext = nullptr;
    AVFormatContext *avFormatContext = nullptr;
    AVFilterContext *avFilterContext = nullptr;
    AVAudioFifo *avAudioFifo = nullptr;
    AVFrame *fifoReadFrame = nullptr; //用于从fifo中读取
    AVFrame *emptyFrame = nullptr; //用于读取失败时写入
    bool isPlanar = false;
    bool isEof = false;

};

class AudioMixer {

private:
    int audioIdGen = 0;

    bool mInitialized = false;

    vector<AudioInfo> mAudioInfos;

    AVFilterGraph *mFilterGraph = nullptr;

    map<int, AudioContext *> mAudioContexts;

    AVFilterContext *mSinkFilterContext = nullptr;

    AVFilterInOut *mSinkIn = nullptr; //sink buffer输入引脚

    AVFilterInOut *mSrcOuts = nullptr;

    static AVFrame *newAudioFrame(AVCodecContext *avCodecContext, bool fill);

    void writeFifo(AudioContext *audioContext);

    void addFrame2Buffer();

    bool writeAudioFrame2Fiwrifo(AudioContext *audioContext);

    void writeEmptyFrame2Fifo(AudioContext *audioContext);

    static long ptsToUs(AVRational timeBase, int64_t pts);

    static int64_t usToPts(AVRational timeBase, long us);

public:
    AudioMixer();

    virtual ~AudioMixer();

    bool addAudio(const string &filePath);

    bool init();

    void reset();

    AudioPCM *readFrame();

};


#endif //FFMPEG_SAMPLE_AUDIOMIXER_H
