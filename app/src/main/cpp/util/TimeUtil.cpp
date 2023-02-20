//
// Created by vecrates on 2023/2/14.
//

#include <ctime>
#include "TimeUtil.h"

TimeUtil::TimeUtil() = default;

long long TimeUtil::timestampMilliSec() {
    struct timeval tv{};
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
}

long long TimeUtil::timestampMicroSec() {
    struct timeval tv{};
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}
