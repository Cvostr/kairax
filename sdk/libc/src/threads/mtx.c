#include "threads.h"
#include "sys/futex.h"
#include "errno.h"

#define THREAD_SELF __errno_location()

int __mtx_trylock(mtx_t* mutex, int* lockval);

int mtx_init(mtx_t* mutex, int type)
{
    mutex->lock = 0;
    mutex->type = type;
    mutex->owner = 0;
    return thrd_success;
}

int mtx_lock(mtx_t* mutex)
{
    return mtx_timedlock(mutex, NULL);
}

int mtx_timedlock(mtx_t* mutex, const struct timespec* time)
{
    int rc, lval;
    do {
        rc = __mtx_trylock(mutex, &lval);
        if (rc != thrd_busy) {
            return rc;
        }

        while (1) {
            rc = futex(&mutex->lock, FUTEX_WAIT, lval, time);
            if (rc == 0) {
                break;
            } else {
                if (errno == EAGAIN) {
                    rc = 0;
                    break;
                } else if (errno == EINTR) {
                    continue;
                } else if (errno == ETIMEDOUT) {
                    return thrd_timedout;
                }
                // todo: implement for signals
            }
        } 
    } while (rc == 0);

    return thrd_error;
}

int __mtx_trylock(mtx_t* mutex, int* lockval)
{
    void *self = THREAD_SELF;

    int lockv;
    if ((lockv = __sync_val_compare_and_swap(&mutex->lock, 0, 1)) == 0) {
        mutex->owner = self;
		return thrd_success;
    }

    if ((mutex->type & mtx_recursive) && mutex->owner == self) {
        if (__sync_add_and_fetch(&mutex->lock, 1) > 1000) {
            __sync_add_and_fetch(&mutex->lock, -1);
            return thrd_error;
        }
        return thrd_success;
    }

    if (lockval)
        *lockval = lockv;

    return thrd_busy;
}

int mtx_trylock(mtx_t* mutex)
{
    return __mtx_trylock(mutex, NULL);
}

int mtx_unlock(mtx_t* mutex)
{
    if (mutex->lock == 0) {
        return thrd_error;
    }

    if (mutex->type & mtx_recursive && mutex->owner == THREAD_SELF) {

        if (__sync_add_and_fetch(&mutex->lock, -1) == 0) {
            mutex->owner = 0;
            futex(&mutex->lock, FUTEX_WAKE, 1, NULL);
        }

        return thrd_success;
    }

    if (__sync_val_compare_and_swap(&mutex->lock, 1, 0) == 1) {
		futex(&mutex->lock, FUTEX_WAKE, 1, NULL);
    }

    return thrd_success;
}

void mtx_destroy(mtx_t* mutex)
{
    // ничего не делаем : )
}