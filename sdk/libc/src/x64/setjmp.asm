[BITS 64]
[SECTION .text]

%define RBX_OFFSET  0x0
%define RBP_OFFSET  0x8
%define R12_OFFSET  0x10
%define R13_OFFSET  0x18
%define R14_OFFSET  0x20
%define R15_OFFSET  0x28
%define RSP_OFFSET  0x30
%define RIP_OFFSET  0x38

global __sigsetjmp
__sigsetjmp:
    mov qword [rdi + RBX_OFFSET], rbx
    mov qword [rdi + RBP_OFFSET], rbp
    mov qword [rdi + R12_OFFSET], r12
    mov qword [rdi + R13_OFFSET], r13 
    mov qword [rdi + R14_OFFSET], r14
    mov qword [rdi + R15_OFFSET], r15
    ; rsp
    lea qword rdx, [rsp + 0x8]
    mov qword [rdi + RSP_OFFSET], rdx
    ; rip
    mov qword rdx, [rsp]
    mov qword [rdi + RIP_OFFSET], rdx
    
    mov rax, 0
    ret

global longjmp
longjmp:
    ; rip
    mov qword rdx, [rdi + RIP_OFFSET]

    mov qword rbx, [rdi + RBX_OFFSET]
    mov qword rbp, [rdi + RBP_OFFSET]
    mov qword r12, [rdi + R12_OFFSET]
    mov qword r13, [rdi + R13_OFFSET]
    mov qword r14, [rdi + R14_OFFSET]
    mov qword r15, [rdi + R15_OFFSET]
    ; rsp
    mov qword rsp, [rdi + RSP_OFFSET]

    ; Если передали 0, то должны вернуть 1
    cmp rsi, 0
    jne .exit
    mov rsi, 1  

.exit:
    mov rax, rsi

    jmp rdx