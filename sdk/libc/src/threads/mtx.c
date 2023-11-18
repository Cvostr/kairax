#include "threads.h"

int mtx_init(mtx_t* mutex, int type)
{
    mutex->lock = 0;
    mutex->type = type;
}

int mtx_lock(mtx_t* mutex)
{
    /*if (!__sync_val_compare_and_swap(mutex->lock, 0, 1) == 0)
	{
		asm volatile("pause");
	}*/
}

int mtx_trylock(mtx_t* mutex)
{

}

int mtx_unlock(mtx_t* mutex)
{

}

void mtx_destroy(mtx_t* mutex)
{
    
}