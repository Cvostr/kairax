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

global syscall_process_get_id
global syscall_thread_get_id
global syscall_get_working_dir
global syscall_set_working_dir
global syscall_process_exit
global syscall_process_exit
global syscall_set_file_mode
global syscall_create_thread
global syscall_create_process
global syscall_poweroff
global syscall_create_directory
global syscall_process_map_memory
global syscall_process_unmap_memory
global syscall_get_time_epoch
global syscall_set_time_epoch
global syscall_thread_exit
global syscall_get_ticks_count
global syscall_load_module
global syscall_unload_module
global syscall_futex

DEFINE_SYSCALL syscall_read,        0x0
DEFINE_SYSCALL syscall_write,       0x1
DEFINE_SYSCALL syscall_open_file,   0x2
DEFINE_SYSCALL syscall_close,       0x3
DEFINE_SYSCALL syscall_fdstat,      0x5
DEFINE_SYSCALL syscall_wait,        0x7
DEFINE_SYSCALL syscall_file_seek,   0x8
DEFINE_SYSCALL syscall_ioctl,       0x10
DEFINE_SYSCALL syscall_create_pipe, 0x16
DEFINE_SYSCALL syscall_sched_yield, 0x18
DEFINE_SYSCALL syscall_sleep,       0x23
DEFINE_SYSCALL syscall_socket,      0x29
DEFINE_SYSCALL syscall_sendto,      0x2C
DEFINE_SYSCALL syscall_recvfrom,    0x2D
DEFINE_SYSCALL syscall_rename,      0x52
DEFINE_SYSCALL syscall_rmdir,       0x54
DEFINE_SYSCALL syscall_unlink,      0x57
DEFINE_SYSCALL syscall_readdir,     0x59
DEFINE_SYSCALL syscall_mount,       0xA5

syscall_process_map_memory:
    mov rax, 0x9
    mov r10, rcx
    syscall
    ret

syscall_process_unmap_memory:
    mov rax, 0xB
    mov r10, rcx
    syscall
    ret

syscall_process_get_id:
    mov rax, 0x27
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

syscall_poweroff:
    mov rax, 0xA9
    syscall
    ret

syscall_load_module:
    mov rax, 0xAF
    mov r10, rcx
    syscall
    ret

syscall_unload_module:
    mov rax, 0xB0
    mov r10, rcx
    syscall
    ret

syscall_process_exit:
    mov rax, 0x3C
    syscall

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

syscall_futex:
    mov rax, 0xCA
    mov r10, rcx
    syscall
    ret

syscall_create_process:
    mov rax, 0x300
    mov r10, rcx
    syscall
    ret