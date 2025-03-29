#ifndef _TIMER_H
#define _TIMER_H

#include "kairax/time.h"

#define TIMER_FREQUENCY 500

struct event_timer 
{
    struct timespec when;
    int alarmed;

    struct thread* wait_head;
    struct thread* wait_tail;
};

void timer_init();
void timer_handle(); // Главная функция-обработчик прерывания таймера
struct event_timer* new_event_timer();
struct event_timer* register_event_timer(struct timespec duration);
void sleep_on_timer(struct event_timer* timer);
void unregister_event_timer(struct event_timer* timer);
void timer_get_ticks(struct timespec* ticks);
time_t timer_get_uptime();

#endif