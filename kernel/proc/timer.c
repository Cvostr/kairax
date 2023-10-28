#include "timer.h"
#include "string.h"
#include "mem/kheap.h"

struct timespec current_counted_time = {0, 0};

void timer_handle()
{
    struct timespec offset = {
        .tv_sec = 0, 
        .tv_nsec = 1000000000 / TIMER_FREQUENCY
    };

    timespec_add(&current_counted_time, &offset);
}

struct event_timer* new_event_timer()
{
    struct event_timer* timer = kmalloc(sizeof(struct event_timer));
    memset(timer, 0, sizeof(struct event_timer));
    return timer;
}

struct event_timer* register_event_timer(struct timespec duration)
{
    struct timespec when = current_counted_time;
    timespec_add(&when, &duration);

    struct event_timer* timer = new_event_timer();
    timer->when = when;

    // TODO: add to array
}