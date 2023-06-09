[BITS 64]
[SECTION .text]

extern sysc
global syscall_handler

%macro pushaq 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro
%macro popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

syscall_handler:
    ;cli
    swapgs
    mov [gs: 0], rsp    ; запоминание стека процесса
    mov rsp, [gs: 8]    ; Установка стека ядра

    pushaq

    mov rdi, rax
    call sysc

    popaq

    mov [gs: 8], rsp
    mov rsp, [gs: 0]    ; Возвращаем стек процесса

    swapgs
    ;sti
    o64 sysret
