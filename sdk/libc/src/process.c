#include "process.h"
#include "syscalls.h"
#include <stdint.h>
#include "errno.h"

pid_t create_thread(void* entry, void* arg)
{
    __set_errno(syscall_create_thread(entry, arg, 0));
}

pid_t create_thread_ex(void* entry, void* arg, size_t stack_size)
{
    __set_errno(syscall_create_thread(entry, arg, stack_size));
}

pid_t process_get_id()
{
    return (pid_t)syscall_process_get_id();
}

pid_t thread_get_id()
{
    return (pid_t)syscall_thread_get_id();
}

void sleep(int time)
{
    syscall_sleep(time);
}