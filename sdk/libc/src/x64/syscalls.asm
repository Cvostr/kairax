[BITS 64]
[SECTION .text]

global syscall_process_get_id
global syscall_thread_get_id
global syscall_get_working_dir
global syscall_set_working_dir
global syscall_process_exit
global syscall_sleep
global syscall_scheduler_yield
global syscall_process_exit
global syscall_read
global syscall_write
global syscall_ioctl
global syscall_open_file
global syscall_close
global syscall_rename
global syscall_unlink
global syscall_fdstat
global syscall_readdir
global syscall_file_seek
global syscall_create_pipe
global syscall_set_file_mode
global syscall_create_thread
global syscall_create_process
global syscall_mount
global syscall_poweroff
global syscall_create_directory
global syscall_process_map_memory
global syscall_process_unmap_memory
global syscall_wait
global syscall_get_time_epoch
global syscall_thread_exit
global syscall_get_ticks_count
global syscall_load_module
global syscall_unload_module

syscall_read:
    mov rax, 0x0
    syscall
    ret

syscall_write:
    mov rax, 0x1
    syscall
    ret

syscall_open_file:
    mov rax, 0x02
    mov r10, rcx
    syscall
    ret

syscall_close:
    mov rax, 0x03
    syscall
    ret

syscall_fdstat:
    mov rax, 0x05
    mov r10, rcx
    syscall
    ret

syscall_file_seek:
    mov rax, 0x08
    syscall
    ret

syscall_create_pipe:
    mov rax, 0x16
    syscall
    ret

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

syscall_ioctl:
    mov rax, 0x10
    syscall
    ret

syscall_readdir:
    mov rax, 0x59
    syscall
    ret

syscall_rename:
    mov rax, 0x52
    mov r10, rcx
    syscall
    ret

syscall_unlink:
    mov rax, 0x57
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

syscall_create_directory:
    mov rax, 0x53
    syscall
    ret
    
syscall_set_file_mode:
    mov rax, 0x5A
    mov r10, rcx
    syscall
    ret

syscall_mount:
    mov rax, 0xA5
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

syscall_wait:
    mov rax, 0x7
    mov r10, rcx
    syscall
    ret

syscall_get_time_epoch:
    mov rax, 0x60
    mov r10, rcx
    syscall
    ret

syscall_create_process:
    mov rax, 0x300
    mov r10, rcx
    syscall
    ret