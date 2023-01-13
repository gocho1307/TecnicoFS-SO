/*
 *      File: better-locks.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Wrappers for some pthread.h functions that WARN when
 *                   something goes wrong.
 */

#include "logging.h"
#include <errno.h>
#include <pthread.h>
#include <string.h>

/**
 * Initializes a rwlock. When unsuccessful, it exits with failure.
 */
void rwlock_init(pthread_rwlock_t *lock) {
    if (pthread_rwlock_init(lock, NULL) != 0) {
        PANIC("Failed to initialize rwlock: %s", strerror(errno));
    }
}

/**
 * Destroys a rwlock. When unsuccessful, it exits with failure.
 */
void rwlock_destroy(pthread_rwlock_t *lock) {
    if (pthread_rwlock_destroy(lock) != 0) {
        PANIC("Failed to destroy rwlock: %s", strerror(errno));
    }
}

/**
 * Locks a rwlock to read-only. When unsuccessful, it exits with failure.
 */
void rwlock_rdlock(pthread_rwlock_t *lock) {
    if (pthread_rwlock_rdlock(lock) != 0) {
        PANIC("Failed to lock rwlock to read-only: %s", strerror(errno));
    }
}

/**
 * Locks a rwlock to write-read. When unsuccessful, it exits with failure.
 */
void rwlock_wrlock(pthread_rwlock_t *lock) {
    if (pthread_rwlock_wrlock(lock) != 0) {
        PANIC("Failed to lock rwlock to write-read: %s", strerror(errno));
    }
}

/**
 * Unlocks a rwlock. When unsuccessful, it exits with failure.
 */
void rwlock_unlock(pthread_rwlock_t *lock) {
    if (pthread_rwlock_unlock(lock) != 0) {
        PANIC("Failed to unlock rwlock: %s", strerror(errno));
    }
}

/**
 * Initializes a mutex. When unsuccessful, it exits with failure.
 */
void mutex_init(pthread_mutex_t *mutex) {
    if (pthread_mutex_init(mutex, NULL) != 0) {
        PANIC("Failed to initialize mutex: %s", strerror(errno));
    }
}

/**
 * Destroys a mutex. When unsuccessful, it exits with failure.
 */
void mutex_destroy(pthread_mutex_t *mutex) {
    if (pthread_mutex_destroy(mutex) != 0) {
        PANIC("Failed to destroy mutex: %s", strerror(errno));
    }
}

/**
 * Locks a mutex. When unsuccessful, it exits with failure.
 */
void mutex_lock(pthread_mutex_t *mutex) {
    if (pthread_mutex_lock(mutex) != 0) {
        PANIC("Failed to lock mutex: %s", strerror(errno));
    }
}

/**
 * Unlocks a mutex. When unsuccessful, it exits with failure.
 */
void mutex_unlock(pthread_mutex_t *mutex) {
    if (pthread_mutex_unlock(mutex) != 0) {
        PANIC("Failed to unlock mutex: %s", strerror(errno));
    }
}

/**
 * Initializes a conditional variable. When unsuccessful, it exits with failure.
 */
void cond_init(pthread_cond_t *cond) {
    if (pthread_cond_init(cond, NULL) != 0) {
        PANIC("Failed to initialize conditional variable: %s", strerror(errno));
    }
}

/**
 * Destroys a conditional variable. When unsuccessful, it exits with failure.
 */
void cond_destroy(pthread_cond_t *cond) {
    if (pthread_cond_destroy(cond) != 0) {
        PANIC("Failed to destroy conditional variable: %s", strerror(errno));
    }
}

/**
 * Waits for a conditional variable. When unsuccessful, it exits with failure.
 */
void cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    if (pthread_cond_wait(cond, mutex) != 0) {
        PANIC("Failed to wait for conditional variable: %s", strerror(errno));
    }
}

/**
 * Signals a conditional variable. When unsuccessful, it exits with failure.
 */
void cond_signal(pthread_cond_t *cond) {
    if (pthread_cond_signal(cond) != 0) {
        PANIC("Failed to signal conditional variable: %s", strerror(errno));
    }
}

/**
 * Broadcasts a conditional variable. When unsuccessful, it exits with failure.
 */
void cond_broadcast(pthread_cond_t *cond) {
    if (pthread_cond_broadcast(cond) != 0) {
        PANIC("Failed to broadcast conditional variable: %s", strerror(errno));
    }
}
