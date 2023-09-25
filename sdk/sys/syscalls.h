#ifndef SYSCALLS_H
#define SYSCALLS_H

extern int syscall_open_file(int dirfd, const char* filepath, int flags, int mode);
extern int syscall_close(int fd);
extern long long syscall_read(int fd, char* buffer, unsigned long long size);
extern long long syscall_write(int fd, const char* buffer, unsigned long long size);
extern int syscall_ioctl(int fd, unsigned long long request, unsigned long long arg);
extern int syscall_readdir(int fd, void* buffer);
extern unsigned long syscall_process_get_id();
extern unsigned long int syscall_thread_get_id();
extern void syscall_sleep(unsigned long long times);
extern int syscall_get_working_dir(char* buffer, unsigned long long size);
extern int syscall_set_working_dir(const char* path);
extern int syscall_fdstat(int dirfd, const char* filepath, void* stat_buffer, int flags);
extern long int syscall_file_seek(int fd, long int offset, int whence);
extern int syscall_create_pipe(int* dirfd, int flags);
extern int syscall_set_file_mode(int dirfd, const char* filepath, unsigned int mode, int flags);
extern int syscall_mount(const char* device, const char* mount_dir, const char* fs);
extern int syscall_create_directory(int dirfd, const char* pathname, int mode);
extern void* syscall_process_map_memory(void* address, unsigned long long length, int protection, int flags);
extern void* syscall_process_unmap_memory(void* address, unsigned long long length);
extern int syscall_poweroff(int cmd);

extern unsigned long int syscall_create_thread(void* entry, void* arg, unsigned long long* tid, unsigned long long stack_size);

#endif