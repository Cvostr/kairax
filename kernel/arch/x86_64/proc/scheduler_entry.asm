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
    mov ax, fs
    push ax
    ; переключение на сегмент ядра
    mov ax, KERNEL_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax

    ; Переход в код планировщика
    mov rdi, rsp
    cld
    call scheduler_handler
    mov rsp, rax

    ; Извлечь значения сегментных регистров fs, es, ds
    pop ax
    mov fs, ax
    pop ax
    mov es, ax
    pop ax
    mov ds, ax
    ; Извлечь значения основных регистров
    popaq
    iretq