#include "proc/thread_scheduler.h"

#define MAX_SYSCALLS 1000

#define DEFINE_SYSCALL(num, func) [num] = func

void* syscall_table[MAX_SYSCALLS] = {
    DEFINE_SYSCALL(0x0, process_read_file),
    DEFINE_SYSCALL(0x1, process_write_file),
    DEFINE_SYSCALL(0x2, process_open_file),
    DEFINE_SYSCALL(0x3, process_close_file),
    DEFINE_SYSCALL(0x5, process_stat),
    DEFINE_SYSCALL(0x8, process_file_seek),
    DEFINE_SYSCALL(0x10, process_ioctl),
    DEFINE_SYSCALL(0x18, scheduler_yield)
};