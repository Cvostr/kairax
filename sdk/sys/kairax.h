#ifndef KAIRAX_H
#define KAIRAX_H

typedef unsigned long long  id_t;
typedef void*               handle_t;

#include "stat.h"

extern int syscall_open_file(const char* filepath, int flags, int mode);
extern int syscall_close(int fd);
extern int syscall_read(int fd, char* buffer, unsigned long long size);
extern id_t syscall_process_get_id();
extern id_t syscall_thread_get_id();
extern void syscall_sleep(unsigned long long times);
extern int syscall_get_working_dir(char* buffer, unsigned long long size);
extern int syscall_set_working_dir(const char* path);
extern int syscall_fdstat(int fd, struct stat* st);

#define FILE_OPEN_MODE_READ_ONLY    1
#define FILE_OPEN_MODE_WRITE_ONLY   2
#define FILE_OPEN_MODE_READ_WRITE   3

#endif