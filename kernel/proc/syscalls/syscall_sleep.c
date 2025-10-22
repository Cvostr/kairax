#include "proc/syscalls.h"
#include "proc/timer.h"
#include "mem/kheap.h"
#include "cpu/cpu_local.h"

int sys_thread_sleep(time_t sec, long int nsec)
{
    int rc = 0;

    if (sec == 0 && nsec == 0) {
        goto exit;
    }

    if (sec < 0 || nsec < 0) {
        rc = -ERROR_INVALID_VALUE;
        goto exit;
    }

    struct thread* thread = cpu_get_current_thread();
    struct timespec duration = {.tv_sec = sec, .tv_nsec = nsec};
    struct event_timer* timer = register_event_timer(duration);
    if (timer == NULL) {
        return -ENOMEM;
    }
    
    if (sleep_on_timer(timer) == WAKE_SIGNAL)
    {
        rc = -EINTR;
    }

    unregister_event_timer(timer);
    kfree(timer);

exit:
    return rc;
}

int sys_pause()
{
    scheduler_sleep1();
    return -EINTR;
}