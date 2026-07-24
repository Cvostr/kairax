#ifndef _EVENT_H
#define _EVENT_H

#include "kairax/atomic.h"
#include "spinlock.h"

struct thread;

struct event {

    spinlock_t lock;
    unsigned int state;

    struct thread*  first;
    struct thread*  last;
};

void event_init(struct event *ev, int state);
void event_reset(struct event *ev);

void event_wait(struct event *ev);
void event_complete(struct event *ev);

#endif