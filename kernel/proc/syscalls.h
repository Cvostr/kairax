#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include "fs/vfs/stat.h"
#include "fs/vfs/dirent.h"

int sys_open_file(int dirfd, const char* path, int flags, int mode);

int sys_mkdir(int dirfd, const char* path, int mode);

int sys_close_file(int fd);

ssize_t sys_read_file(int fd, char* buffer, size_t size);

ssize_t sys_write_file(int fd, const char* buffer, size_t size);

int sys_stat(int dirfd, const char* filepath, struct stat* statbuf, int flags);

int sys_readdir(int fd, struct dirent* dirent);

int sys_ioctl(int fd, uint64_t request, uint64_t arg);

off_t sys_file_seek(int fd, off_t offset, int whence); 

int sys_get_working_dir(char* buffer, size_t size);

int sys_set_working_dir(const char* buffer);

pid_t sys_get_process_id();

pid_t sys_get_thread_id();

void sys_exit_process(int code);

int sys_thread_sleep(uint64_t time);

int sys_create_thread(void* entry_ptr, void* arg, pid_t* tid, size_t stack_size);

#endif