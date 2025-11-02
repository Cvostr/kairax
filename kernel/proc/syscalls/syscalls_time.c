#include "proc/syscalls.h"
#include "proc/timer.h"
#include "cpu/cpu_local.h"

// Архитектурно-специфично
void arch_get_time_epoch(struct timeval *tv);
void arch_set_time_epoch(const struct timeval *tv);

int sys_get_time_epoch_protected(struct timeval *tv)
{
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, tv, sizeof(struct timeval))

    return sys_get_time_epoch(tv);
}

int sys_get_time_epoch(struct timeval *tv)
{
    arch_get_time_epoch(tv);
    return 0;
}

int sys_set_time_epoch(const struct timeval *tv)
{
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, tv, sizeof(struct timeval))

    if (tv->tv_sec < 0 || tv->tv_usec < 0 || tv->tv_usec > 1000000) {
        return -ERROR_INVALID_VALUE;
    }

    arch_set_time_epoch(tv);
    return 0;
}

uint64_t sys_get_tick_count()
{
    struct timespec ticks;
    timer_get_ticks(&ticks);
    uint64_t t = ticks.tv_sec * 1000 + ticks.tv_nsec / 1000000;
    return t;
}