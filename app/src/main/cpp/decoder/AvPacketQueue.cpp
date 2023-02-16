//
// Created by vecrates on 2022/12/17.
//

#include <thread>
#include <android/log.h>
#include "AvPacketQueue.h"

AvPacketQueue::AvPacketQueue(int size) {
    this->mSize = size;
    this->mQueue = new std::queue<AVPacket *>();
}

AvPacketQueue::~AvPacketQueue() {
    clear();
    delete mQueue;
}

void AvPacketQueue::clear() {
    while (!mQueue->empty()) {
        AVPacket *packet = mQueue->front();
        av_packet_free(&packet);
        mQueue->pop();
    }
}

bool AvPacketQueue::push(AVPacket *packet) {
    if (packet == nullptr) {
        return false;
    }
    if (isFull()) {
        return false;
    }
    mQueue->push(packet);
    return true;
}

AVPacket *AvPacketQueue::pop() {
    AVPacket *packet = nullptr;
    if (!mQueue->empty()) {
        packet = mQueue->front();
        mQueue->pop();
    }
    return packet;
}

bool AvPacketQueue::isEmpty() {
    return mQueue->empty();
}

bool AvPacketQueue::isFull() {
    return mQueue->size() >= mSize;
}

int AvPacketQueue::size() {
    return mQueue->size();
}
