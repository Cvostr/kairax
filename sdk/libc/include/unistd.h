#ifndef _UNISTD_H
#define _UNISTD_H

#include "sys/types.h"
#include "stddef.h"
#include "sys/cdefs.h"

__BEGIN_DECLS

#ifndef _STDIO_H
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

pid_t fork(void);

pid_t vfork(void);

int execve(const char *filename, char *const argv [], char *const envp[]);  
int execv(const char *file, char *const argv[]);
int execvp(const char *file, char *const argv[]);

int dup(int oldfd);
int dup2(int oldfd, int newfd);

uid_t getuid(void);
uid_t geteuid(void);
int setuid(uid_t uid);

gid_t getgid(void);
gid_t getegid(void);
int setgid(gid_t gid);

pid_t getpid(void);
pid_t getppid(void);
pid_t gettid(void);

int setpgid(pid_t pid, pid_t pgid) __THROW;
pid_t getpgid(pid_t pid) __THROW;
int setpgrp(void) __THROW;
pid_t getpgrp(void) __THROW;

ssize_t read(int fd, char* buffer, size_t size);

ssize_t write(int fd, const char* buffer, size_t size);

int close(int fd);

int unlink(const char* path);

int rmdir(const char *path);

int usleep(unsigned long useconds);
unsigned int sleep(unsigned int seconds);

int pause(void) __THROW;

off_t lseek(int fd, off_t offset, int whence);

int link(const char *oldpath, const char *newpath);
int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);

int symlink(const char *target, const char *linkpath);
int symlinkat(const char *target, int newdirfd, const char *linkpath);

ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsize) __THROW;
ssize_t readlink(const char *path, char *buf, size_t bufsize) __THROW;

char* getcwd(char* buf, size_t size);

int chdir(const char* path);

int pipe(int pipefd[2]);

int isatty(int fd);
pid_t tcgetpgrp(int fd) __THROW;
int tcsetpgrp(int fd, pid_t pgrp) __THROW;

void _exit(int status);

void* sbrk(int len);

#define _SC_OPEN_MAX 4
#define _SC_PAGESIZE 5
long sysconf(int name);

int gethostname(char *name, size_t len) __THROW;
int sethostname(const char *name, size_t len) __THROW;

int getdomainname(char *name, size_t len) __THROW;
int setdomainname(const char *name, size_t len) __THROW;

__END_DECLS

#endif