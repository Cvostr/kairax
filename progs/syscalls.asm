[BITS 64]
[SECTION .text]

global printf

printf:
    mov rax, 1
    syscall
    ret