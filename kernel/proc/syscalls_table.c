#include "proc/thread_scheduler.h"
#include "syscalls.h"

#define MAX_SYSCALLS 1000

#define DEFINE_SYSCALL(num, func) [num] = func

void* syscalls_table[MAX_SYSCALLS] = {
    DEFINE_SYSCALL(0x0, sys_read_file),
    DEFINE_SYSCALL(0x1, sys_write_file),
    DEFINE_SYSCALL(0x2, sys_open_file),
    DEFINE_SYSCALL(0x3, sys_close_file),
    DEFINE_SYSCALL(0x5, sys_stat),
    DEFINE_SYSCALL(0x8, sys_file_seek),
    DEFINE_SYSCALL(0x10, sys_ioctl),
    DEFINE_SYSCALL(0x18, scheduler_yield),
    DEFINE_SYSCALL(0x23, sys_thread_sleep),
    DEFINE_SYSCALL(0x27, sys_get_process_id),
    DEFINE_SYSCALL(0x3C, sys_exit_process),
    DEFINE_SYSCALL(0xBA, sys_get_thread_id),
    DEFINE_SYSCALL(0x4F, sys_get_working_dir),
    DEFINE_SYSCALL(0x50, sys_set_working_dir),
    DEFINE_SYSCALL(0x53, sys_mkdir),
    DEFINE_SYSCALL(0x59, sys_readdir),

    DEFINE_SYSCALL(0x2FF, sys_create_thread)
};