#include "threads.h"
#include "sys/futex.h"
#include "errno.h"
#include "limits.h"

int cnd_init(cnd_t* cond)
{
    cond->cnd = 0;
    return thrd_success;
}

int cnd_signal(cnd_t *cond)
{
    // TODO: figure out
    futex(&cond->cnd, FUTEX_WAKE, 1, NULL);
    return thrd_success;
}

int cnd_broadcast(cnd_t *cond)
{
    // TODO: figure out
    futex(&cond->cnd, FUTEX_WAKE, INT_MAX, NULL);
    return thrd_success;
}

int cnd_wait(cnd_t* cond, mtx_t* mutex)
{
    return cnd_timedwait(cond, mutex, NULL);
}

int cnd_timedwait(cnd_t* restrict cond, mtx_t* restrict mutex, const struct timespec* restrict time_point)
{
    // todo: implement, check
    int rc;
    mtx_unlock(mutex);

    do {
        rc = futex(&cond->cnd, FUTEX_WAIT, 1, time_point);
        if (rc == -1) {
            if (errno == EAGAIN) {
                break;
            } else {
                break;
            }
        }

    } while (rc == 0);

    cond->cnd = 0;

    return mtx_timedlock(mutex, time_point);
}

void cnd_destroy(cnd_t* cond)
{
    // ничего не делаем : )
}