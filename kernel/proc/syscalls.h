#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include "fs/vfs/stat.h"
#include "fs/vfs/dirent.h"
#include "process.h"
#include "time.h"

int sys_not_implemented();

int sys_open_file(int dirfd, const char* path, int flags, int mode);

int sys_mkdir(int dirfd, const char* path, int mode);

int sys_close_file(int fd);

ssize_t sys_read_file(int fd, char* buffer, size_t size);

ssize_t sys_write_file(int fd, const char* buffer, size_t size);

int sys_stat(int dirfd, const char* filepath, struct stat* statbuf, int flags);

int sys_set_mode(int dirfd, const char* filepath, mode_t mode, int flags);

int sys_readdir(int fd, struct dirent* dirent);

int sys_ioctl(int fd, uint64_t request, uint64_t arg);

off_t sys_file_seek(int fd, off_t offset, int whence); 

int sys_pipe(int* pipefd, int flags);

int sys_get_working_dir(char* buffer, size_t size);

int sys_set_working_dir(const char* buffer);

pid_t sys_get_process_id();

pid_t sys_get_thread_id();

void sys_exit_process(int code);

int sys_thread_sleep(uint64_t time);

void* sys_memory_map(void* address, uint64_t length, int protection, int flags);

int sys_memory_protect(void* address, uint64_t length, int protection);

int sys_memory_unmap(void* address, uint64_t length);

int sys_mount(const char* device, const char* mount_dir, const char* fs);

pid_t sys_create_thread(void* entry_ptr, void* arg, size_t stack_size);

pid_t sys_create_process(int dirfd, const char* filepath, struct process_create_info* info);

int sys_poweroff(int cmd);

pid_t sys_wait(int mode, pid_t id, int* status, int options);

int sys_get_time_epoch(struct timeval *tv);

#endif