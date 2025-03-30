#ifndef SYSCALLS_H
#define SYSCALLS_H

int syscall_netctl(int op, int param, void* netinfo);
int syscall_routectl(int type, unsigned int action, int arg, void* route);
int syscall_netstat(int index, void* stat);

int syscall_load_module(const unsigned char* module_image, size_t image_size);
int syscall_unload_module(const char* module_name);

int syscall_sysinfo(int request, void* buffer, size_t bufsize);

#endif