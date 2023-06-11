[BITS 64]
[SECTION .text]

extern syscall_handle
global syscall_entry_x64

%macro push_regs 0
    push r11
    push rcx

    push rax
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r12
    push r13
    push r14
    push r15
%endmacro
%macro pop_regs 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rax

    pop rcx
    pop r11
%endmacro

; rax - номер вызова
; rcx - (SYSRET) - адрес инструкции возврата
; r11 - (SYSRET) - флаги возврата
; rdi - аргумент 1
; rsi - аргумент 2
; rdx - 3
; r8 - 4
; r9 - 5
; 10 - 6

syscall_entry_x64:
    cli
    swapgs
    mov [gs : 0], rsp    ; запоминание стека процесса
    mov rsp, [gs : 8]    ; Установка стека ядра

    push_regs            ; Запомнить основные регистры

    mov rdi, rsp
    call syscall_handle

    pop_regs

    mov [gs : 8], rsp
    mov rsp, [gs : 0]    ; Возвращаем стек процесса

    swapgs
    sti
    o64 sysret
