#include "process.h"
#include "syscalls.h"
#include <stdint.h>
#include "errno.h"

pid_t create_thread(void* entry, void* arg)
{
    __sys_ret_set_errno(syscall_create_thread(entry, arg, 0));
}

pid_t create_thread_ex(void* entry, void* arg, size_t stack_size)
{
    __sys_ret_set_errno(syscall_create_thread(entry, arg, stack_size));
}

void thread_exit(int code)
{
    syscall_thread_exit(code);
}

pid_t create_process(int dirfd, const char* filepath, struct process_create_info* info)
{
    __sys_ret_set_errno(syscall_create_process(dirfd, filepath, info));
}