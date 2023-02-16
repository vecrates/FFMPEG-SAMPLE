//
// Created by vecrates on 2022/12/17.
//

#ifndef FFMPEG_SAMPLE_AVPACKETQUEUE_H
#define FFMPEG_SAMPLE_AVPACKETQUEUE_H

#include <queue>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/packet.h"

#ifdef __cplusplus
}
#endif

class AvPacketQueue {

public:
    AvPacketQueue(int size);

    ~AvPacketQueue();

    bool push(AVPacket *packet);

    AVPacket *pop();

    void clear();

    bool isEmpty();

    bool isFull();

    int size();


private:


    std::queue<AVPacket *> *mQueue;

    int mSize = 516;

};


#endif //FFMPEG_SAMPLE_AVPACKETQUEUE_H
