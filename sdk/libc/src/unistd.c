#include "unistd.h"
#include "syscalls.h"
#include "errno.h"

pid_t getpid()
{
    return syscall_process_get_id();
}

pid_t gettid()
{
    return syscall_thread_get_id();
}

unsigned int sleep(unsigned int seconds)
{
    int rc = syscall_sleep(seconds, 0);
    return 0;
}

char* getcwd(char* buf, size_t size)
{
    int rc = syscall_get_working_dir(buf, size);
    if (rc != 0) {
        errno = -rc;
        return NULL;
    }

    return buf;
}

int chdir(const char* path)
{
    __set_errno(syscall_set_working_dir(path));
}

int pipe(int pipefd[2])
{
    __set_errno(syscall_create_pipe(pipefd, 0));
}

int close(int fd)
{
    __set_errno(syscall_close(fd));
}

off_t lseek(int fd, off_t offset, int whence)
{
    __set_errno(syscall_file_seek(fd, offset, whence));
}

ssize_t read(int fd, char* buffer, size_t size)
{
    __set_errno(syscall_read(fd, buffer, size));
}

ssize_t write(int fd, const char* buffer, size_t size)
{
    __set_errno(syscall_write(fd, buffer, size));
}

void _exit(int status)
{
    syscall_process_exit(status);
}