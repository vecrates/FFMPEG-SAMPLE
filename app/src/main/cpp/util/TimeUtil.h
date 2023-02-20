//
// Created by vecrates on 2023/2/14.
//

#ifndef FFMPEG_SAMPLE_TIMEUTIL_H
#define FFMPEG_SAMPLE_TIMEUTIL_H


class TimeUtil {

private:
    TimeUtil();

public:
    static long long timestampMilliSec();

    static long long timestampMicroSec();

};


#endif //FFMPEG_SAMPLE_TIMEUTIL_H
