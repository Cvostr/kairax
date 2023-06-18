[BITS 64]
[SECTION .text]

extern main
extern syscall_process_exit
global _start

_start:
    call main
    mov rdi, rax    ; Поместить возвращаемое значение как первый аргумент
    jmp syscall_process_exit
