#ifndef MUTEX_H
#define MUTEX_H

#include "common.h"
#include <pthread.h>

struct mutex
{
    pthread_mutex_t lock;
};

static inline __attribute__((unused)) void mutex_init(struct mutex * mutex)
{
    pthread_mutex_init(&mutex->lock, NULL);
}

static inline __attribute__((unused)) void mutex_uninit(struct mutex * mutex)
{
    pthread_mutex_destroy(&mutex->lock);
}

static inline __attribute__((unused)) void mutex_lock(struct mutex * mutex)
{
    pthread_mutex_lock(&mutex->lock);
}

static inline __attribute__((unused)) void mutex_unlock(struct mutex * mutex)
{
    pthread_mutex_unlock(&mutex->lock);
}

#endif