//
// Created by vecrates on 2022/12/1.
//

#include "BaseDecoder.h"

BaseDecoder::BaseDecoder(AVFormatContext *ftx, int streamIndex) {
    this->mAvFormatContext = ftx;
    this->mStreamIndex = streamIndex;
    this->mTimeBase = mAvFormatContext->streams[streamIndex]->time_base;
    this->mDuration = ptsToUs(mAvFormatContext->streams[streamIndex]->duration);
}

BaseDecoder::~BaseDecoder() {
    mAvFormatContext = nullptr;
}

bool BaseDecoder::init() {
    return false;
}

void BaseDecoder::decode(AVPacket *packet) {

}

void BaseDecoder::flush() {

}

int BaseDecoder::getStreamIndex() {
    return mStreamIndex;
}

long BaseDecoder::getCurrentTimestamp() {
    return mCurrentTimestamp;
}

long BaseDecoder::ptsToUs(int64_t pts) {
    return pts * av_q2d(mTimeBase) * 1000000L;
}

