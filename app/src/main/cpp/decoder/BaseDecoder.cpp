//
// Created by vecrates on 2022/12/1.
//

#include "BaseDecoder.h"

#include <utility>

BaseDecoder::BaseDecoder(AVFormatContext *ftx, int streamIndex) {
    this->mAvFormatContext = ftx;
    this->mStreamIndex = streamIndex;
    this->mTimeBase = mAvFormatContext->streams[streamIndex]->time_base;
    this->mDuration = mAvFormatContext->streams[streamIndex]->duration * av_q2d(mTimeBase);
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

void BaseDecoder::setFrameAvailableListener(std::function<void(AVFrame *)> listener) {
    this->mFrameAvailableListener = std::move(listener);
}

long BaseDecoder::getCurrentTimestamp() {

}
