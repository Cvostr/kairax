#ifndef _TIMER_H
#define _TIMER_H

#include "kairax/time.h"

#define TIMER_FREQUENCY 250

struct event_timer 
{
    struct timespec when;
    struct thread* wait_head;
    int alarmed;
    int sig;
};

void timer_init();
void timer_handle(); // Главная функция-обработчик прерывания таймера
struct event_timer* new_event_timer();
struct event_timer* create_and_register_event_timer(struct timespec duration);
void bind_to_timer(struct event_timer* timer);
int sleep_on_timer(struct event_timer* timer);
void register_event_timer(struct event_timer* timer);
void unregister_event_timer(struct event_timer* timer);
void timer_get_ticks(struct timespec* ticks);
time_t timer_get_uptime();

#endif