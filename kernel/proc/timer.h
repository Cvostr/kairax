#ifndef _TIMER_H
#define _TIMER_H

#include "stdc/time.h"

#define TIMER_FREQUENCY 1000

struct event_timer {
    void* handle; // Для ожиданий потоков
    struct timespec when;
    int alarmed;
};

void timer_init();
void timer_handle(); // Главная функция-обработчик прерывания таймера
struct event_timer* new_event_timer();
struct event_timer* register_event_timer(struct timespec duration);
void unregister_event_timer(struct event_timer* timer);

#endif