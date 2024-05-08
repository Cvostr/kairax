#ifndef SYSCALLS_H
#define SYSCALLS_H

extern int syscall_open_file(int dirfd, const char* filepath, int flags, int mode);
extern int syscall_close(int fd);
extern long long syscall_read(int fd, char* buffer, unsigned long long size);
extern long long syscall_write(int fd, const char* buffer, unsigned long long size);
extern int syscall_ioctl(int fd, unsigned long long request, unsigned long long arg);
extern int syscall_readdir(int fd, void* buffer);
extern long syscall_process_get_id();
extern long syscall_thread_get_id();
extern int syscall_sleep(long long sec, long long nsec);
extern int syscall_get_working_dir(char* buffer, unsigned long long size);
extern int syscall_set_working_dir(const char* path);
extern int syscall_fdstat(int dirfd, const char* filepath, void* stat_buffer, int flags);
extern long long int syscall_file_seek(int fd, long int offset, int whence);
extern int syscall_create_pipe(int* dirfd, int flags);
extern int syscall_set_file_mode(int dirfd, const char* filepath, unsigned int mode, int flags);
extern int syscall_unlink(int fd, const char* path, int flags);
extern int syscall_rmdir(const char* path);
extern int syscall_rename(int olddirfd, const char* oldpath, int newdirfd, const char* newpath);
extern int syscall_mount(const char* device, const char* mount_dir, const char* fs);
extern int syscall_create_directory(int dirfd, const char* pathname, int mode);
extern void* syscall_process_map_memory(void* address, unsigned long long length, int protection, int flags);
extern int syscall_process_unmap_memory(void* address, unsigned long long length);
extern void syscall_process_exit(int status);
extern int syscall_poweroff(int cmd);
extern long long int syscall_wait(long int pid, int* status, int options);
extern int syscall_get_time_epoch(void *tv);
extern int syscall_set_time_epoch(const void *tv);
extern int syscall_futex(int* addr, int op, int val, const void* timeout);

extern int syscall_load_module(void* module_image, unsigned long long image_size);
extern int syscall_unload_module(const char* module_name);

extern long long int syscall_create_thread(void* entry, void* arg, unsigned long long stack_size);
extern long long int syscall_create_process(int dirfd, const char* filepath, void* info);
extern void syscall_thread_exit(int code);
extern unsigned long long int syscall_get_ticks_count();
extern int syscall_sched_yield();

// --- SOCKET ---
extern int syscall_socket(int domain, int type, int protocol);

extern int syscall_sendto(int sockfd, const void *msg, unsigned long long len, int flags, const void* to, unsigned int tolen);

extern long long syscall_recvfrom(int sockfd, void* buf, unsigned long long len, int flags, const void* src_addr, unsigned int* addrlen);

#endif