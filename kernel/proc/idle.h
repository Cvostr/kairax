#ifndef _IDLE_H
#define _IDLE_H

#include "thread.h"

void init_idle_process();
struct thread* create_idle_thread();

#endif