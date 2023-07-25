#ifndef _PIPE_H
#define _PIPE_H

#include "types.h"
#include "atomic.h"
#include "sync/spinlock.h"

struct pipe {

    char*       buffer;
    atomic_t    ref_count;

    size_t      size;
    loff_t      write_pos;
    loff_t      read_pos;
};

struct pipe* new_pipe(size_t size);


#endif