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

pid_t getpid(void);

pid_t getppid(void);

pid_t gettid(void);

ssize_t read(int fd, char* buffer, size_t size);

ssize_t write(int fd, const char* buffer, size_t size);

int close(int fd);

int unlink(const char* path);

int rmdir(const char *path);

int usleep(unsigned long useconds);
unsigned int sleep(unsigned int seconds);

off_t lseek(int fd, off_t offset, int whence);

char* getcwd(char* buf, size_t size);

int chdir(const char* path);

int pipe(int pipefd[2]);

int isatty(int fd);

void _exit(int status);

void* sbrk(int len);

__END_DECLS

#endif