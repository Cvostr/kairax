[BITS 64]
[SECTION .text]

%macro DEFINE_SYSCALL 2
    global %1
    %1:
        mov rax, %2
        mov r10, rcx
        syscall
        ret
%endmacro

global syscall_thread_get_id
global syscall_get_working_dir
global syscall_set_working_dir
global syscall_set_file_mode
global syscall_create_thread
global syscall_create_process
global syscall_create_directory
global syscall_process_unmap_memory
global syscall_get_time_epoch
global syscall_set_time_epoch
global syscall_thread_exit
global syscall_get_ticks_count
global syscall_unload_module

DEFINE_SYSCALL syscall_read,        0x0
DEFINE_SYSCALL syscall_write,       0x1
DEFINE_SYSCALL syscall_open_file,   0x2
DEFINE_SYSCALL syscall_close,       0x3
DEFINE_SYSCALL syscall_fdstat,      0x5
DEFINE_SYSCALL syscall_wait,        0x7
DEFINE_SYSCALL syscall_file_seek,   0x8
DEFINE_SYSCALL syscall_map_memory,  0x9
DEFINE_SYSCALL syscall_protect_memory,  0xA
DEFINE_SYSCALL syscall_ioctl,       0x10
DEFINE_SYSCALL syscall_create_pipe, 0x16
DEFINE_SYSCALL syscall_sched_yield, 0x18

DEFINE_SYSCALL syscall_dup,         0x20
DEFINE_SYSCALL syscall_dup2,        0x21

DEFINE_SYSCALL syscall_sleep,       0x23
DEFINE_SYSCALL syscall_getpid,      0x27
DEFINE_SYSCALL syscall_socket,      0x29
DEFINE_SYSCALL syscall_connect,     0x2A
DEFINE_SYSCALL syscall_accept,      0x2B
DEFINE_SYSCALL syscall_sendto,      0x2C
DEFINE_SYSCALL syscall_recvfrom,    0x2D
DEFINE_SYSCALL syscall_shutdown,    0x30
DEFINE_SYSCALL syscall_bind,        0x31
DEFINE_SYSCALL syscall_listen,      0x32
DEFINE_SYSCALL syscall_getpeername, 0x34
DEFINE_SYSCALL syscall_setsockopt,  0x36
DEFINE_SYSCALL syscall_fork,        0x39
DEFINE_SYSCALL syscall_vfork,       0x3A
DEFINE_SYSCALL syscall_execve,      0x3B
DEFINE_SYSCALL syscall_process_exit, 0x3C
DEFINE_SYSCALL syscall_kill,        0x3E
DEFINE_SYSCALL syscall_fcntl,       0x48
DEFINE_SYSCALL syscall_readdir,     0x4E
DEFINE_SYSCALL syscall_rename,      0x52
DEFINE_SYSCALL syscall_rmdir,       0x54
DEFINE_SYSCALL syscall_linkat,      0x56
DEFINE_SYSCALL syscall_unlink,      0x57
DEFINE_SYSCALL syscall_symlinkat,   0x58
DEFINE_SYSCALL syscall_readlinkat,  0x59
DEFINE_SYSCALL syscall_getuid,      0x66
DEFINE_SYSCALL syscall_getgid,      0x68
DEFINE_SYSCALL syscall_setuid,      0x69
DEFINE_SYSCALL syscall_setgid,      0x6A
DEFINE_SYSCALL syscall_geteuid,     0x6B
DEFINE_SYSCALL syscall_getegid,     0x6C
DEFINE_SYSCALL syscall_getppid,     0x6E
DEFINE_SYSCALL syscall_sigprocmask, 0x7E
DEFINE_SYSCALL syscall_sigpending,  0x7F
DEFINE_SYSCALL syscall_mknodat,     0x85
DEFINE_SYSCALL syscall_mount,       0xA5
DEFINE_SYSCALL syscall_poweroff,    0xA9

DEFINE_SYSCALL syscall_load_module, 0xAF

DEFINE_SYSCALL syscall_futex,       0xCA

DEFINE_SYSCALL syscall_create_pty,  0x303

syscall_process_unmap_memory:
    mov rax, 0xB
    mov r10, rcx
    syscall
    ret

syscall_thread_get_id:
    mov rax, 0xBA
    syscall
    ret

syscall_process_set_break:
    mov rax, 0xC
    syscall
    ret

syscall_get_working_dir:
    mov rax, 0x4F
    syscall
    ret

syscall_set_working_dir:
    mov rax, 0x50
    syscall
    ret

syscall_create_directory:
    mov rax, 0x53
    syscall
    ret
    
syscall_set_file_mode:
    mov rax, 0x5A
    mov r10, rcx
    syscall
    ret

syscall_unload_module:
    mov rax, 0xB0
    mov r10, rcx
    syscall
    ret

syscall_create_thread:
    mov rax, 0x2FF
    mov r10, rcx
    syscall
    ret

syscall_thread_exit:
    mov rax, 0x301
    mov r10, rcx
    syscall
    ret

syscall_get_ticks_count:
    mov rax, 0x302
    mov r10, rcx
    syscall
    ret

syscall_get_time_epoch:
    mov rax, 0x60
    mov r10, rcx
    syscall
    ret

syscall_set_time_epoch:
    mov rax, 0xA4
    mov r10, rcx
    syscall
    ret

syscall_create_process:
    mov rax, 0x300
    mov r10, rcx
    syscall
    ret