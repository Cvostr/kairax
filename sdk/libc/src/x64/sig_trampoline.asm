[BITS 64]
[SECTION .text]

global __sig_trampoline

__sig_trampoline:
    mov rax, 0xF
    syscall
    ret