#include "threads.h"

int mtx_init(mtx_t* mutex, int type)
{
    mutex->lock = 0;
    mutex->type = type;
}

int mtx_lock(mtx_t* mutex)
{
    
}

int __mtx_trylock(mtx_t* mutex, int* lockval)
{
    int lockv;
    if ((lockv = __sync_val_compare_and_swap(&mutex->lock, 0, 1)) == 0)
	{
		return thrd_success;
    }

    if ((mutex->type & mtx_recursive) /*&&  проверка владения*/) {
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
    if (mutex->lock == 0)
    {
        return thrd_error;
    }
}

void mtx_destroy(mtx_t* mutex)
{
    
}