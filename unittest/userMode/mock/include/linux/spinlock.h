#ifndef SPIN_LOCK_H
#define SPIN_LOCK_H

#include <pthread.h>

typedef pthread_spinlock_t spinlock_t;

#define spin_lock_init(lock) pthread_spin_init(&(lock), PTHREAD_PROCESS_PRIVATE)
#define spin_lock(lock) pthread_spin_lock(lock)
#define spin_unlock(lock) pthread_spin_unlock(lock)
#define spin_lock_uninit(lock) pthread_spin_destroy(&lock)

#endif