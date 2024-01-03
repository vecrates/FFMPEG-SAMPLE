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
    mFrameAvailableListener = nullptr;
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

AVRational BaseDecoder::getTimebase() {
    return mAvFormatContext->streams[getStreamIndex()]->time_base;
}

long BaseDecoder::ptsToUs(int64_t pts) {
    return pts * av_q2d(mTimeBase) * 1000000L;
}

long BaseDecoder::usToPts(long us) {
    float s = us / (float) 1000000;
    return s / av_q2d(mTimeBase);
}

void BaseDecoder::setFrameAvailableListener(std::function<void(AVFrame *)> listener) {
    this->mFrameAvailableListener = std::move(listener);
}

void BaseDecoder::seekTo(long us) {

}

