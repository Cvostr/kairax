%include "memory/mem_layout.asm"

[BITS 64]
[SECTION .text]

extern syscalls_table
extern process_handle_signals
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
; rdx - аргумент 3
; r10 - аргумент 4
; r8 - аргумент 5
; r9 - аргумент 6

%define RAX_OFFSET 96

syscall_entry_x64:
    swapgs
    
    mov [gs : 0], rsp    ; запоминание стека процесса
    mov rsp, [gs : 8]    ; Установка стека ядра

    push_regs            ; Запомнить основные регистры

    sti                  ; Включение прерываний

    cmp rax, 800
    ja syscalls_exit

    mov rcx, r10
    call syscalls_table[rax * 8]
    mov [rsp + RAX_OFFSET], rax

    mov rdi, 3
    mov rsi, rsp
    call process_handle_signals

syscalls_exit:
    cli                 ; Выключение прерываний

    pop_regs

    mov [gs : 8], rsp    ; Запись стека ядра
    mov rsp, [gs : 0]    ; Возвращаем стек процесса

    swapgs
    
    o64 sysret