[BITS 64]
[SECTION .text]

global printf
global syscall_process_exit

printf:
    mov rax, 1
    syscall
    ret

syscall_process_pid:
    mov rax, 0x27
    syscall
    ret

syscall_process_set_break:
    mov rax, 0xC
    syscall
    ret

syscall_process_exit:
    mov rax, 0x100
    syscall
stub:
    jmp stub