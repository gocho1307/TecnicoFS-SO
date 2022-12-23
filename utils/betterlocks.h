#ifndef __UTILS_BETTER_LOCKS_H__
#define __UTILS_BETTER_LOCKS_H__

#include <pthread.h>

void rwlock_init(pthread_rwlock_t *lock);
void rwlock_destroy(pthread_rwlock_t *lock);
void rwlock_rdlock(pthread_rwlock_t *lock);
void rwlock_wrlock(pthread_rwlock_t *lock);
void rwlock_unlock(pthread_rwlock_t *lock);

void mutex_init(pthread_mutex_t *mutex);
void mutex_destroy(pthread_mutex_t *mutex);
void mutex_lock(pthread_mutex_t *mutex);
void mutex_unlock(pthread_mutex_t *mutex);

#endif // __UTILS_BETTER_LOCKS_H__
