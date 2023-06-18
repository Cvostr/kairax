#ifndef KAIRAX_H
#define KAIRAX_H

typedef unsigned long long  id_t;
typedef void*               handle_t;

extern int syscall_open_file(const char* filepath, int flags, int mode);
extern int syscall_read(int fd, char* buffer, unsigned long long size);
extern id_t syscall_process_get_id();
extern id_t syscall_thread_get_id();
extern void syscall_sleep(unsigned long long times);
extern int syscall_get_working_dir(char* buffer, unsigned long long size);
int syscall_set_working_dir(const char* path);

#endif