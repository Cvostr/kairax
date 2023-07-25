[BITS 64]
[SECTION .text]

global syscall_printf
global syscall_process_get_id
global syscall_thread_get_id
global syscall_get_working_dir
global syscall_process_exit
global syscall_sleep
global syscall_scheduler_yield
global syscall_read
global syscall_open_file
global syscall_close
global syscall_fdstat
global syscall_readdir
global syscall_file_seek
global syscall_set_file_mode
global syscall_create_thread
global syscall_mount

syscall_printf:
    mov rax, 1
    syscall
    ret

syscall_read:
    mov rax, 0x0
    syscall
    ret

syscall_open_file:
    mov rax, 0x02
    syscall
    ret

syscall_close:
    mov rax, 0x03
    syscall
    ret

syscall_fdstat:
    mov rax, 0x05
    syscall
    ret

syscall_file_seek:
    mov rax, 0x08
    syscall
    ret

syscall_readdir:
    mov rax, 0x59
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

syscall_scheduler_yield:
    mov rax, 0x18
    syscall
    ret

syscall_sleep:
    mov rax, 0x23
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

syscall_set_file_mode:
    mov rax, 0x5A
    syscall
    ret

syscall_mount:
    mov rax, 0xA5
    syscall
    ret

syscall_process_exit:
    mov rax, 0x3C
    syscall

syscall_create_thread:
    mov rax, 0xFF10
    syscall
    ret