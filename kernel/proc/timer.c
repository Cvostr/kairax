#include "timer.h"
#include "string.h"
#include "mem/kheap.h"
#include "sync/spinlock.h"
#include "list/list.h"
#include "thread_scheduler.h"

struct timespec current_ticks = {0, 0}; // Время, вычисляемое обработчиком прерывания таймера
spinlock_t timers_lock = 0;
list_t* timers_list = NULL;

extern void arch_timer_init();

void timer_init()
{
    timers_list = create_list();
    arch_timer_init();
}

void timer_get_ticks(struct timespec* ticks)
{
    ticks->tv_sec = current_ticks.tv_sec;
    ticks->tv_nsec = current_ticks.tv_nsec;
}

void timer_handle()
{
    struct timespec offset = {
        .tv_sec = 0, 
        .tv_nsec = 1000000000 / TIMER_FREQUENCY
    };

    timespec_add(&current_ticks, &offset);

    if (try_acquire_spinlock(&timers_lock)) {

        for (size_t i = 0; i < list_size(timers_list); i ++) {
            struct event_timer* ev_timer = list_get(timers_list, i);

            if (ev_timer->alarmed) {
                continue;
            }

            timespec_sub(&ev_timer->when, &offset);

            if (timespec_is_zero(&ev_timer->when)) {
                scheduler_wakeup(ev_timer);
                ev_timer->alarmed = 1;
            }
        }

        release_spinlock(&timers_lock);
    }
}

struct event_timer* new_event_timer()
{
    struct event_timer* timer = kmalloc(sizeof(struct event_timer));
    memset(timer, 0, sizeof(struct event_timer));
    return timer;
}

struct event_timer* register_event_timer(struct timespec duration)
{
    struct event_timer* timer = new_event_timer();
    timer->when = duration;

    acquire_spinlock(&timers_lock);
    list_add(timers_list, timer);
    release_spinlock(&timers_lock);

    return timer;
}

void unregister_event_timer(struct event_timer* timer)
{
    acquire_spinlock(&timers_lock);
    list_remove(timers_list, timer);
    release_spinlock(&timers_lock);
}