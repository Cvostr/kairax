%include "base/regs_stack.asm"
%include "base/swapgs.asm"

[BITS 64]
[SECTION .text]

extern scheduler_handler

;Сегмент данных ядра
%define KERNEL_DATA_SEG 0x10

global scheduler_entry
scheduler_entry:
    _swapgs 8
    ; Поместить все 64-битные регистры в стек
    pushaq
    ; Поместить сегментные регистры в стек
    ;pushsg

    ; Переход в код планировщика
    mov rdi, rsp
    jmp scheduler_handler

global scheduler_exit
scheduler_exit:
    mov rsp, rdi
    ; Извлечь значения сегментных регистров es, ds
    ;popsg
    ; Извлечь значения основных регистров
    popaq
    ; swapgs если необходимо сменить кольцо
    _swapgs 8
    ; Переход на другую задачу
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