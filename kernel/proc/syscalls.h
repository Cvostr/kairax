#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include "fs/vfs/stat.h"
#include "fs/vfs/dirent.h"
#include "process.h"
#include "kairax/time.h"
#include "kairax/in.h"
#include "kairax/signal.h"
#include "kairax/select.h"
#include "kairax/poll.h"

#define VALIDATE_NULL_POINTER(base) if (base == 0) {return -ERROR_INVALID_VALUE;}
#define VALIDATE_USER_POINTER(proc,base,len) if (process_is_userspace_region(proc,base,len,0) == 0) {return -ERROR_INVALID_VALUE;}
#define VALIDATE_USER_POINTER_PROTECTION(proc,base,len,prot) if (process_is_userspace_region(proc,base,len,prot) == 0) {return -ERROR_INVALID_VALUE;}

#define VALIDATE_USER_STRING(proc,str) if (process_is_userspace_string(proc,str) == 0) {return -ERROR_INVALID_VALUE;}

int sys_not_implemented();

int sys_yield();

int sys_open_file(int dirfd, const char* path, int flags, int mode);

int sys_mkdir(int dirfd, const char* path, int mode);

int sys_close_file(int fd);

ssize_t sys_read_file(int fd, char* buffer, size_t size);

ssize_t sys_write_file(int fd, const char* buffer, size_t size);

int sys_stat(int dirfd, const char* filepath, struct stat* statbuf, int flags);

int sys_set_mode(int dirfd, const char* filepath, mode_t mode, int flags);

int sys_readdir(int fd, struct dirent* dirent);

int sys_ioctl(int fd, uint64_t request, uint64_t arg);

int sys_unlink(int dirfd, const char* path, int flags);

int sys_rmdir(const char* path);

int sys_rename(int olddirfd, const char* oldpath, int newdirfd, const char* newpath, int flags);

int sys_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);

int sys_symlinkat(const char *target, int newdirfd, const char *linkpath);

ssize_t sys_readlinkat(int dirfd, const char* pathname, char* buf, size_t bufsize);

int sys_mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev);

off_t sys_file_seek(int fd, off_t offset, int whence); 

int sys_fcntl(int fd, int cmd, unsigned long long arg);

int sys_pipe(int* pipefd, int flags);

int sys_get_working_dir(char* buffer, size_t size);

int sys_set_working_dir(const char* buffer);

pid_t sys_get_process_id();
pid_t sys_get_parent_process_id();

pid_t sys_get_thread_id();

int sys_setpgid(pid_t pid, pid_t pgid);
pid_t sys_getpgid(pid_t pid);

void sys_exit_process(int code);

pid_t sys_fork();
pid_t sys_vfork();
int sys_execve(const char *filename, char *const argv [], char *const envp[]);  

void sys_exit_thread(int code);

int sys_thread_sleep(time_t tv_sec, long int tv_nsec);

int sys_pause();

void* sys_memory_map(void* address, uint64_t length, int protection, int flags, int fd, int offset);

int sys_memory_protect(void* address, uint64_t length, int protection);

int sys_memory_unmap(void* address, uint64_t length);

int sys_mount(const char* device, const char* mount_dir, const char* fs, unsigned long flags);
int sys_unmount(const char* mount_dir, unsigned long flags);

pid_t sys_create_thread(void* entry_ptr, void* arg, size_t stack_size);

pid_t sys_create_process(int dirfd, const char* filepath, struct process_create_info* info);

int sys_poweroff(int cmd);

pid_t sys_wait(pid_t id, int* status, int options);

int sys_get_time_epoch(struct timeval *tv);
int sys_get_time_epoch_protected(struct timeval *tv);
int sys_set_time_epoch(const struct timeval *tv);

uint64_t sys_get_tick_count();

int sys_create_pty(int *master_fd, int *slave_fd);

int sys_load_module(void* module_image, size_t image_size);
int sys_unload_module(const char* module_name);

int sys_futex(void* futex, int op, int val, const struct timespec *timeout);

struct netinfo;
int sys_netctl(int op, int param, struct netinfo* netinfo);

struct netstat;
int sys_netstat(int index, struct netstat* stat);

struct route_info;
int sys_routectl(int type, int action, int arg, struct route_info* route);

int sys_sysinfo(int request, char* buffer, size_t bufsize);

uid_t sys_getuid(void);
uid_t sys_geteuid(void);
int sys_setuid(uid_t uid);

gid_t sys_getgid(void);
gid_t sys_getegid(void);
int sys_setgid(gid_t gid);

int sys_dup(int oldfd);
int sys_dup2(int oldfd, int newfd);

int sys_sethostname(const char *name, size_t len);
int sys_setdomainname(const char *name, size_t len);

int sys_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int sys_poll(struct pollfd *ufds, nfds_t nfds, int timeout);

// ---- SOCKETS ---------
int sys_socket(int domain, int type, int protocol);

int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int sys_listen(int sockfd, int backlog);

int sys_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int sys_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int sys_sendto(int sockfd, const void *msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen);

ssize_t sys_recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* from, socklen_t* addrlen);

int sys_shutdown(int sockfd, int how);
int sys_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int sys_getsockname(int sockfd, struct sockaddr *name, socklen_t *namelen);

// ----- SIGNALS -------

int sys_sigprocmask(int how, const sigset_t * set, sigset_t *oldset, size_t sigsetsize);
int sys_send_signal(pid_t pid, int signal);
int sys_send_signal_pg(pid_t group, int signal);
int sys_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact, size_t sigsetsize);
int sys_sigpending(sigset_t *set, size_t sigsetsize);
int sys_sigreturn();

#endif