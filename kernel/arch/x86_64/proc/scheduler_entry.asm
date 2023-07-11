%include "base/regs_stack.asm"

[BITS 64]
[SECTION .text]

extern scheduler_handler

;Сегмент данных ядра
%define KERNEL_DATA_SEG 0x10

global scheduler_entry
scheduler_entry:
    ; Поместить все 64-битные регистры в стек
    pushaq
    ; Поместить сегментные регистры в стек
    mov ax, ds
    push ax
    mov ax, es
    push ax
    ; переключение на сегмент ядра
    mov ax, KERNEL_DATA_SEG
    mov ds, ax
    mov es, ax

    ; Переход в код планировщика
    mov rdi, rsp
    cld
    call scheduler_handler
    mov rsp, rax

    ; Извлечь значения сегментных регистров es, ds
    pop ax
    mov es, ax
    pop ax
    mov ds, ax
    ; Извлечь значения основных регистров
    popaq
    iretq
    
global scheduler_yield_entry
scheduler_yield_entry:
    pop rdi ; rip
    mov rsi, rsp
    
    xor rax, rax
    ; SS
    mov ax, ss
    push rax
    ; RSP
    push rsi
    ; RFLAGS
    pushfq
    ; CS
    mov ax, cs
    push rax
    ; RIP
    push rdi

    jmp scheduler_entry