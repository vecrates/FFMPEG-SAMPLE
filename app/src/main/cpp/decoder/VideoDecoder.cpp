//
// Created by vecrates on 2022/11/29.
//

#include "VideoDecoder.h"
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/frame.h"
#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"VideoDecoder",__VA_ARGS__)

VideoDecoder::VideoDecoder(AVFormatContext *ftx, int streamIndex)
        : BaseDecoder(ftx, streamIndex) {

}

VideoDecoder::~VideoDecoder() {
    release();
}

void VideoDecoder::release() {
    if (mAvFrame != nullptr) {
        av_frame_free(&mAvFrame);
        mAvFrame = nullptr;
    }
    if (mCodecContext != nullptr) {
        avcodec_close(mCodecContext);
        avcodec_free_context(&mCodecContext);
        mCodecContext = nullptr;
    }
    if (mSwsContext != nullptr) {
        sws_freeContext(mSwsContext);
        mSwsContext = nullptr;
    }
    mFrameAvailableListener = nullptr;
    mVideoCodec = nullptr;
}

bool VideoDecoder::init() {
    AVStream *stream = mAvFormatContext->streams[getStreamIndex()];
    AVCodecParameters *codecPar = stream->codecpar;
    mWidth = codecPar->width;
    mHeight = codecPar->height;

    //找到对应的解码器
    mVideoCodec = avcodec_find_decoder(codecPar->codec_id);
    if (mVideoCodec == nullptr) {
        LOGE("avcodec_find_decoder() invoke failed");
        return false;
    }

    //创建CodecContext
    mCodecContext = avcodec_alloc_context3(mVideoCodec);
    //将码流参数复制到CodecContext
    int ret = avcodec_parameters_to_context(mCodecContext, codecPar);
    if (ret < 0) {
        LOGE("#avcodec_parameters_to_context, result=%d", ret);
        return false;
    }

    //打开解码器
    ret = avcodec_open2(mCodecContext, mVideoCodec, nullptr);
    if (ret < 0) {
        LOGE("#avcodec_open2, result=%d", ret);
        return false;
    }

    //创建AvFrame，存储解码后的帧
    mAvFrame = av_frame_alloc();

    mAvFrameRGB = av_frame_alloc();


    return true;
}

void VideoDecoder::updateSws(int mWidth, int mHeight) {
    AVStream *stream = mAvFormatContext->streams[getStreamIndex()];
    AVCodecParameters *codecPar = stream->codecpar;
    mSwsContext = sws_getContext(codecPar->width, codecPar->height, AVPixelFormat(codecPar->format), //输入
                                 mWidth, mHeight, AV_PIX_FMT_RGBA, //输出
                                 SWS_FAST_BILINEAR, //选择缩放算法(只有当输入输出图像大小不同时有效),一般选择SWS_FAST_BILINEAR
                                 nullptr,  /* 输入图像的滤波器信息, 若不需要传NULL */
                                 nullptr, /* 输出图像的滤波器信息, 若不需要传NULL */
                                 nullptr /* 特定缩放算法需要的参数(?)，默认为NULL */
    );

    //计算 Buffer 的大小
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, mWidth, mHeight, 1);
    //为 m_RGBAFrame 分配空间
    mFrameBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
    //格式化，会将pFrameRGB的数据按RGB格式自动"关联"到buffer，即pFrameRGB中的数据改变了，out_buffer中的数据也会相应的改变
    av_image_fill_arrays(mAvFrameRGB->data, mAvFrameRGB->linesize, mFrameBuffer,
                         AV_PIX_FMT_RGBA, mWidth, mHeight, 1);
}

void VideoDecoder::decode(AVPacket *packet) {
    int ret = avcodec_send_packet(mCodecContext, packet);
    if (ret != 0 && ret != AVERROR(EAGAIN)) {
        LOGE("#avcodec_send_packet, error=%d", ret);
        return;
    }

    ret = avcodec_receive_frame(mCodecContext, mAvFrame);
    if (ret != 0) {
        LOGE("#avcodec_receive_frame, error=%d", ret);
        av_frame_unref(mAvFrame);
        return;
    }

    //3. 格式转换
    sws_scale(mSwsContext, (uint8_t const *const *) mAvFrame->data, mAvFrame->linesize,
              0, mHeight, mAvFrameRGB->data, mAvFrameRGB->linesize);

    if (mFrameAvailableListener != nullptr) {
        mFrameAvailableListener(mAvFrameRGB);
    }

    av_frame_unref(mAvFrame);
}

void VideoDecoder::flush() {
    BaseDecoder::flush();
}

int VideoDecoder::getWidth() {
    return mWidth;
}

int VideoDecoder::getHeight() {
    return mHeight;
}
