#include "timer.h"
#include "string.h"
#include "mem/kheap.h"
#include "sync/spinlock.h"
#include "list/list.h"
#include "thread_scheduler.h"

#include "cpu/cpu_local.h"

struct timespec current_ticks = {0, 0}; // Время, вычисляемое обработчиком прерывания таймера
spinlock_t timers_lock = 0;
list_t* timers_list = NULL;

extern void arch_timer_init();

void timer_init()
{
    timers_list = create_list();
    arch_timer_init();
}

time_t timer_get_uptime()
{
    return current_ticks.tv_sec;
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
                //scheduler_wakeup_intrusive(&ev_timer->wait_head, &ev_timer->wait_tail, NULL, INT_MAX);
                
                struct thread* thr = ev_timer->wait_head;
                if (thr) 
                {
                    thr->timer = NULL;
                    scheduler_wakeup1(thr);
                }
                
                ev_timer->alarmed = 1;
            }
        }

        release_spinlock(&timers_lock);
    }
}

struct event_timer* new_event_timer()
{
    struct event_timer* timer = kmalloc(sizeof(struct event_timer));
    if (timer == NULL) {
        return NULL;
    }
    memset(timer, 0, sizeof(struct event_timer));
    return timer;
}

struct event_timer* register_event_timer(struct timespec duration)
{
    struct event_timer* timer = new_event_timer();
    if (timer == NULL) {
        return NULL;
    }
    timer->when = duration;

    acquire_spinlock(&timers_lock);
    list_add(timers_list, timer);
    release_spinlock(&timers_lock);

    return timer;
}

int sleep_on_timer(struct event_timer* timer)
{   
    struct thread* thread = cpu_get_current_thread();
    timer->wait_head = cpu_get_current_thread();
    thread->timer = timer;
    return scheduler_sleep1();
}

void unregister_event_timer(struct event_timer* timer)
{
    acquire_spinlock(&timers_lock);
    list_remove(timers_list, timer);
    release_spinlock(&timers_lock);
}