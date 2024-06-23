//
// Created by vecrates on 2024/1/8.
//

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/libavcodec/avcodec.h"
#include "../include/libavutil/avutil.h"

#ifdef __cplusplus
};
#endif

void printSupportCodec() {
    char info[40000] = {0};
    void *i = nullptr;
    const AVCodec *c_temp = nullptr;
    while (true) {
        c_temp = av_codec_iterate(&i);
        if (c_temp == nullptr) {
            break;
        }

        if (c_temp->decode != nullptr) {
            sprintf(info, "%s[Dec]", info);
        } else {
            sprintf(info, "%s[Enc]", info);
        }

        switch (c_temp->type) {
            case AVMEDIA_TYPE_VIDEO:
                sprintf(info, "%s[Video]", info);
                break;
            case AVMEDIA_TYPE_AUDIO:
                sprintf(info, "%s[Audio]", info);
                break;
            default:
                sprintf(info, "%s[Other]", info);
                break;
        }
        sprintf(info, "%s %10s\n", info, c_temp->name);
    }

}