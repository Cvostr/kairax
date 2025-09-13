#include "unistd.h"
#include "syscalls.h"
#include "errno.h"
#include "time.h"
#include "fcntl.h"
#include "termios.h"
#include "sys/ioctl.h"
#include "sys/mman.h"
#include "string.h"

pid_t fork(void)
{
    __set_errno(syscall_fork());
}

pid_t vfork(void)
{
    __set_errno(syscall_vfork());
}

int dup(int oldfd)
{
    __set_errno(syscall_dup(oldfd));
}

int dup2(int oldfd, int newfd)
{
    __set_errno(syscall_dup2(oldfd, newfd));
}

void* sbrk(int len)
{
    // TODO: Should use syscall?
    char *mm = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
	memset(mm, 0, len);
    return mm;
}

uid_t getuid(void)
{
    __set_errno(syscall_getuid());
}

uid_t geteuid(void)
{
    __set_errno(syscall_geteuid());
}

int setuid(uid_t uid)
{
    __set_errno(syscall_setuid(uid));
}

gid_t getgid(void)
{
    __set_errno(syscall_getgid());
}

gid_t getegid(void)
{
    __set_errno(syscall_getegid());
}

int setgid(gid_t gid)
{
    __set_errno(syscall_setgid(gid));
}

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

int setpgid(pid_t pid, pid_t pgid)
{
    __set_errno(syscall_setpgid(pid, pgid));
}

pid_t getpgid(pid_t pid)
{
    __set_errno(syscall_getpgid(pid));
}

int setpgrp(void)
{
    return setpgid(0, 0);
}

pid_t getpgrp(void)
{
    return getpgid(0);
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

int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags)
{
    __set_errno(syscall_linkat(AT_FDCWD, oldpath, newdirfd, newpath, flags));
}

int link(const char *oldpath, const char *newpath)
{
    __set_errno(syscall_linkat(AT_FDCWD, oldpath, AT_FDCWD, newpath, 0));
}

int symlinkat(const char *target, int newdirfd, const char *linkpath)
{
    __set_errno(syscall_symlinkat(target, newdirfd, linkpath));
}

int symlink(const char *target, const char *linkpath)
{
    return symlinkat(target, AT_FDCWD, linkpath);
}

ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsize)
{
    __set_errno(syscall_readlinkat(dirfd, pathname, buf, bufsize));
}

ssize_t readlink(const char *path, char *buf, size_t bufsize)
{
    return readlinkat(AT_FDCWD, path, buf, bufsize);
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

int isatty(int fd)
{
    struct termios trm;
    int errno_saved = errno;
    int istty = ioctl(fd, TCGETS, (unsigned long long) &trm) == 0;
    errno = errno_saved;

    return istty;
}

pid_t tcgetpgrp(int fd)
{
    int result = -1;
    if (ioctl(fd, TIOCGPGRP, (unsigned long long) &result) == -1)
        return -1;

    return result;
}

int tcsetpgrp(int fd, pid_t pgrp)
{
    return ioctl(fd, TIOCSPGRP, (unsigned long long) &pgrp);
}

void _exit(int status)
{
    syscall_process_exit(status);
}

long sysconf(int name)
{
    switch (name)
    {
        case _SC_PAGESIZE:
            return 4096;
    }

    errno = ENOSYS;
    return -1;
}