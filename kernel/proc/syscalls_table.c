#include "proc/thread_scheduler.h"
#include "syscalls.h"

#define MAX_SYSCALLS 800

#define DEFINE_SYSCALL(num, func) [num] = func

void* syscalls_table[MAX_SYSCALLS + 1] = {
    [0 ... MAX_SYSCALLS] = sys_not_implemented,

    DEFINE_SYSCALL(0x0, sys_read_file),
    DEFINE_SYSCALL(0x1, sys_write_file),
    DEFINE_SYSCALL(0x2, sys_open_file),
    DEFINE_SYSCALL(0x3, sys_close_file),
    DEFINE_SYSCALL(0x5, sys_stat),
    DEFINE_SYSCALL(0x7, sys_wait),
    DEFINE_SYSCALL(0x8, sys_file_seek),
    DEFINE_SYSCALL(0x9, sys_memory_map),
    DEFINE_SYSCALL(0xA, sys_memory_protect),
    DEFINE_SYSCALL(0xB, sys_memory_unmap),
    DEFINE_SYSCALL(0xD, sys_sigaction),
    DEFINE_SYSCALL(0xF, sys_sigreturn),
    DEFINE_SYSCALL(0x10, sys_ioctl),
    DEFINE_SYSCALL(0x16, sys_pipe),
    DEFINE_SYSCALL(0x18, sys_yield),
    DEFINE_SYSCALL(0x20, sys_dup),
    DEFINE_SYSCALL(0x21, sys_dup2),
    DEFINE_SYSCALL(0x23, sys_thread_sleep),
    DEFINE_SYSCALL(0x27, sys_get_process_id),
    DEFINE_SYSCALL(0x29, sys_socket),
    DEFINE_SYSCALL(0x2A, sys_connect),
    DEFINE_SYSCALL(0x2B, sys_accept),
    DEFINE_SYSCALL(0x2C, sys_sendto),
    DEFINE_SYSCALL(0x2D, sys_recvfrom),
    DEFINE_SYSCALL(0x30, sys_shutdown),
    DEFINE_SYSCALL(0x31, sys_bind),
    DEFINE_SYSCALL(0x32, sys_listen),
    DEFINE_SYSCALL(0x36, sys_setsockopt),
    DEFINE_SYSCALL(0x39, sys_fork),
    DEFINE_SYSCALL(0x3A, sys_vfork),
    DEFINE_SYSCALL(0x3B, sys_execve),
    DEFINE_SYSCALL(0x3C, sys_exit_process),
    DEFINE_SYSCALL(0x3E, sys_send_signal),
    DEFINE_SYSCALL(0x48, sys_fcntl),
    DEFINE_SYSCALL(0x4E, sys_readdir),
    DEFINE_SYSCALL(0x4F, sys_get_working_dir),
    DEFINE_SYSCALL(0x50, sys_set_working_dir),
    DEFINE_SYSCALL(0x52, sys_rename),
    DEFINE_SYSCALL(0x53, sys_mkdir),
    DEFINE_SYSCALL(0x54, sys_rmdir),
    DEFINE_SYSCALL(0x56, sys_linkat),
    DEFINE_SYSCALL(0x57, sys_unlink),
    DEFINE_SYSCALL(0x58, sys_symlinkat),
    DEFINE_SYSCALL(0x59, sys_readlinkat),
    DEFINE_SYSCALL(0x5A, sys_set_mode),
    DEFINE_SYSCALL(0x60, sys_get_time_epoch_protected),
    DEFINE_SYSCALL(0x66, sys_getuid),
    DEFINE_SYSCALL(0x68, sys_getgid),
    DEFINE_SYSCALL(0x69, sys_setuid),
    DEFINE_SYSCALL(0x6A, sys_setgid),
    DEFINE_SYSCALL(0x6B, sys_geteuid),
    DEFINE_SYSCALL(0x6C, sys_getegid),
    DEFINE_SYSCALL(0x6E, sys_get_parent_process_id),
    DEFINE_SYSCALL(0x7E, sys_sigprocmask),
    DEFINE_SYSCALL(0x7F, sys_sigpending),
    DEFINE_SYSCALL(0x85, sys_mknodat),
    DEFINE_SYSCALL(0xA4, sys_set_time_epoch),
    DEFINE_SYSCALL(0xA5, sys_mount),
    DEFINE_SYSCALL(0xA9, sys_poweroff),
    DEFINE_SYSCALL(0xAF, sys_load_module),
    DEFINE_SYSCALL(0xB0, sys_unload_module),
    DEFINE_SYSCALL(0xBA, sys_get_thread_id),
    DEFINE_SYSCALL(0xCA, sys_futex),

    // Не POSIX вызовы
    DEFINE_SYSCALL(0x2FF, sys_create_thread),
    DEFINE_SYSCALL(0x300, sys_create_process),
    DEFINE_SYSCALL(0x301, sys_exit_thread),
    DEFINE_SYSCALL(0x302, sys_get_tick_count),
    DEFINE_SYSCALL(0x303, sys_create_pty),
    DEFINE_SYSCALL(0x304, sys_netctl),
    DEFINE_SYSCALL(0x305, sys_routectl),
    DEFINE_SYSCALL(0x306, sys_sysinfo),
    DEFINE_SYSCALL(0x307, sys_netstat)
};