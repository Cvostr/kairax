#ifndef KAIRAX_H
#define KAIRAX_H

extern int syscall_open_file(const char* filepath, int flags, int mode);
extern int syscall_close(int fd);
extern int syscall_read(int fd, char* buffer, unsigned long long size);
extern unsigned long syscall_process_get_id();
extern unsigned long int syscall_thread_get_id();
extern void syscall_sleep(unsigned long long times);
extern int syscall_get_working_dir(char* buffer, unsigned long long size);
extern int syscall_set_working_dir(const char* path);
extern int syscall_fdstat(int fd, void* st);

#endif