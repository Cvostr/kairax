#ifndef _BLOCKER_H
#define _BLOCKER_H

#include "sync/spinlock.h"

struct thread;

struct blocker {
    struct thread*  head;
    struct thread*  tail;
    spinlock_t      lock;
};

#endif