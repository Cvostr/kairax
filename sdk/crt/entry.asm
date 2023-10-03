[BITS 64]
[SECTION .text]

extern main
global _start

_start:
    call main
    mov rdi, rax    ; Поместить возвращаемое значение как первый аргумент
    ; Завершить процесс
    mov rax, 0x3C
    syscall
