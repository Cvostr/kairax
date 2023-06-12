[BITS 64]
[SECTION .text]

global printf
global syscall_process_get_id
global syscall_thread_get_id
global syscall_get_working_dir
global syscall_process_exit
global syscall_sleep

printf:
    mov rax, 1
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

syscall_process_exit:
    mov rax, 0x3C
    syscall
stub:
    jmp stub