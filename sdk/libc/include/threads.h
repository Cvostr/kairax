#ifndef _THREADS_H
#define _THREADS_H

#include <sys/types.h>
#include <sys/time.h>
#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*thrd_start_t)(void* arg);

typedef struct {
    pid_t tid;
    thrd_start_t func;
    void* arg;
} thrd_t;

enum {
    thrd_success = 0,
    thrd_timedout,
    thrd_busy,
    thrd_nomem,
    thrd_error
};

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);

enum {
    mtx_plain = 0,
    mtx_recursive = 1,
    mtx_timed = 2
};

typedef struct {
    int lock;
    int type;
    int owner;
} mtx_t;

int mtx_init(mtx_t* mutex, int type);
int mtx_lock(mtx_t* mutex);
int mtx_timedlock(mtx_t* mutex, const struct timespec* time);
int mtx_trylock(mtx_t* mutex);
int mtx_unlock(mtx_t* mutex);
void mtx_destroy(mtx_t* mutex);

#ifdef __cplusplus
}
#endif

#endif