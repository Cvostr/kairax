#ifndef KAIRAX_H
#define KAIRAX_H

extern int syscall_open_file(const char* filepath, int flags, int mode);
extern int syscall_close(int fd);
extern int syscall_read(int fd, char* buffer, unsigned long long size);
extern int syscall_readdir(int fd, void* buffer);
extern unsigned long syscall_process_get_id();
extern unsigned long int syscall_thread_get_id();
extern void syscall_sleep(unsigned long long times);
extern int syscall_get_working_dir(char* buffer, unsigned long long size);
extern int syscall_set_working_dir(const char* path);
extern int syscall_fdstat(int fd, void* st);
extern long int syscall_file_seek(int fd, long int offset, int whence);
extern int syscall_set_file_mode(int fd, unsigned int mode);
extern int syscall_mount(const char* device, const char* mount_dir, const char* fs);

extern unsigned long int syscall_create_thread(void* entry, void* arg, unsigned long long* tid, unsigned long long stack_size);

#endif