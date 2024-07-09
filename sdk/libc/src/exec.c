#include "unistd.h"
#include "syscalls.h"
#include "errno.h"
#include "string.h"

extern char** __environ;

int execve(const char *filename, char *const argv [], char *const envp[])
{
    __set_errno(syscall_execve(filename, argv, envp));
}

int execv(const char *file, char *const argv[])
{
    return execve(file, argv, __environ);
}