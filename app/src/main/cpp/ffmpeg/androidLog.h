//
// Created by vecrates on 2024/7/21.
//

#ifndef FFMPEG_SAMPLE_ANDROIDLOG_H
#define FFMPEG_SAMPLE_ANDROIDLOG_H

#include <android/log.h>

#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,"FFmpegCmd",__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,"FFmpegCmd",__VA_ARGS__)


#endif //FFMPEG_SAMPLE_ANDROIDLOG_H
