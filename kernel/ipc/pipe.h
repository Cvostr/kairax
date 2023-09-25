#ifndef _PIPE_H
#define _PIPE_H

#include "types.h"
#include "atomic.h"
#include "sync/spinlock.h"
#include "fs/vfs/file.h"

#define PIPE_SIZE 4096

struct pipe {

    char*       buffer;
    atomic_t    ref_count;
    spinlock_t  lock;

    size_t      size;
    loff_t      write_pos;
    loff_t      read_pos;
};

struct pipe* new_pipe();

int pipe_create_files(struct pipe* pipe, int flags, struct file* read_file, struct file* write_file);

void free_pipe(struct pipe* pipe);

ssize_t pipe_file_read(struct file* file, char* buffer, size_t count, loff_t offset);

ssize_t pipe_file_write(struct file* file, const char* buffer, size_t count, loff_t offset);

#endif