//
// Created by weiyusong on 2023/2/10.
//

#include <pthread.h>
#include "Locker.h"

Locker::Locker() {
    pthread_mutex_init(&mMutex, nullptr);
    pthread_cond_init(&mCond, nullptr);
}

Locker::~Locker() {
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCond);
}

void Locker::lock() {
    pthread_mutex_lock(&mMutex);
}

void Locker::unlock() {
    pthread_mutex_unlock(&mMutex);
}

void Locker::wait(long timeMs) {
    if (timeMs == 0) {
        return;
    }
    pthread_mutex_lock(&mMutex);
    waitWithoutLock(timeMs);
    pthread_mutex_unlock(&mMutex);
}

void Locker::waitWithoutLock(long timeMs) {
    if (timeMs > 0) {
        struct timespec abs_time{};
        struct timeval now_time{};
        gettimeofday(&now_time, nullptr);
        int n_sec = now_time.tv_usec * 1000 + (timeMs % 1000) * 1000000L;
        abs_time.tv_nsec = n_sec % 1000000000;
        abs_time.tv_sec = now_time.tv_sec + n_sec / 1000000000L + timeMs / 1000L;
        pthread_cond_timedwait(&mCond, &mMutex, &abs_time);
    } else {
        pthread_cond_wait(&mCond, &mMutex);
    }
}

void Locker::notify() {
    pthread_mutex_lock(&mMutex);
    pthread_cond_signal(&mCond);
    pthread_mutex_unlock(&mMutex);
}