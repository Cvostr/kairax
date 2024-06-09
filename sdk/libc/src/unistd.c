#include "unistd.h"
#include "syscalls.h"
#include "errno.h"
#include "time.h"
#include "fcntl.h"

pid_t getpid(void)
{
    return syscall_getpid();
}

pid_t getppid(void)
{
    return syscall_getppid();
}

pid_t gettid(void)
{
    return syscall_thread_get_id();
}

unsigned int sleep(unsigned int seconds)
{
    int rc = syscall_sleep(seconds, 0);
    return 0;
}

int usleep(unsigned long useconds)
{
    struct timespec tmscp;
    tmscp.tv_sec = useconds / 1000000;
    tmscp.tv_nsec = (useconds % 1000000) * 1000;
    return nanosleep(&tmscp, &tmscp);
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

int unlink(const char* path)
{
    __set_errno(syscall_unlink(AT_FDCWD, path, 0));
}

int rmdir(const char *path)
{
    __set_errno(syscall_rmdir(path));
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