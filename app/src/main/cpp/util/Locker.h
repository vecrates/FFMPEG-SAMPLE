//
// Created by weiyusong on 2023/2/10.
//

#ifndef FFMPEG_SAMPLE_LOCKER_H
#define FFMPEG_SAMPLE_LOCKER_H

#include <bits/pthread_types.h>

class Locker {

public:

    Locker();

    ~Locker();

    void lock();

    void unlock();

    void wait(long timeMs);

    void waitWithoutLock(long timeMs);

    void notify();

private:

    pthread_mutex_t mMutex{};

    pthread_cond_t mCond{};

};


#endif //FFMPEG_SAMPLE_LOCKER_H
