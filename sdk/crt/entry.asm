[BITS 64]
[SECTION .text]

extern main
global _start

_start:
    mov [__environ], rdx
    call main
    mov rdi, rax    ; Поместить возвращаемое значение как первый аргумент
    ; Завершить процесс
    mov rax, 0x3C
    syscall

section .data
__environ: dq 0
global __environ