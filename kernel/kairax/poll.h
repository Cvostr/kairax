#ifndef _POLL_H
#define _POLL_H

#include "kairax/list/list.h"
#include "sync/spinlock.h"

#define POLLIN      0x001
#define POLLPRI     0x002
#define POLLOUT     0x004
#define POLLERR     0x008
#define POLLHUP     0x010
#define POLLNVAL    0x020
#define POLLRDNORM  0x040
#define POLLWRNORM	0x100

struct file;

struct pollfd {
    int fd;
    short events;
    short revents;
};

typedef unsigned int nfds_t;

// Структура, описывающая очередь ожидающих потоков
struct poll_wait_queue {
    spinlock_t lock;
    list_t threads;
};

struct poll_queued {
    struct file* file;
    struct poll_wait_queue* queue;
    struct poll_queued* next;
    struct poll_queued* prev;
};

// Структура с состоянием для poll/select
struct poll_ctl {
    struct poll_queued      *head;
    struct poll_queued      *tail;
};

void poll_wait(struct poll_ctl* ctl, struct file* file, struct poll_wait_queue* q);
void poll_wakeall(struct poll_wait_queue* q);
void poll_unwait(struct poll_ctl *pctl);

#endif
