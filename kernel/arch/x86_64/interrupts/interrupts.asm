%include "memory/mem_layout.asm"
%include "base/regs_stack.asm"
%include "base/swapgs.asm"

[BITS 64]
[SECTION .text]

section .bss
global idt_descriptors ;таблица дескрипторов прерываний
idt_descriptors:
	resb 4096

extern int_handler ;Обработчик прерываний
extern timer_int_handler

;Сегмент данных ядра
%define KERNEL_DATA_SEG 0x10

section .text

isr_entry:
    _swapgs 24
    ; Поместить все 64-битные регистры в стек
    pushaq
    ; Поместить сегментные регистры в стек
    ;pushsg
    ; Перейти к обработчику
    mov rdi, rsp
    cld
    call int_handler

    ; Извлечь значения сегментных регистров es, ds
    ;popsg
    ; Извлечь 64 битные регистры из стека
    popaq

    _swapgs 24

    add rsp, 16 ; Вытащить код ошибки из стека
    
    iretq

%macro isr_wrapper 1
isr_%1:
    
    %ifn (%1 == 8) || ((%1 >= 10 ) && (%1 <= 14)) || (%1 == 17) || (%1 == 21)
        ; Это не exception -> пишем 0
        push 0
    %endif
    ; Помещаем номер прерываний
    push %1
    jmp isr_entry
%endmacro

isr_32:
    _swapgs 8
    ; Поместить все 64-битные регистры в стек
    pushaq
    ; Поместить сегментные регистры в стек
    ;pushsg
    ; Переключение на сегмент ядра
    ;mov ax, KERNEL_DATA_SEG
    ;mov ds, ax
    ;mov es, ax
    ; Перейти к обработчику
    mov rdi, rsp
    cld
    call timer_int_handler
    ; Переключение задачи не произошло - возвращаемся назад
    ; Извлечь значения сегментных регистров es, ds
    ;popsg
    ; Извлечь 64 битные регистры из стека
    popaq

    _swapgs 8
    
    iretq


%assign i 0
%rep 256
    %if i != 32
    isr_wrapper i
    %endif

    %assign i i+1
%endrep

section .rodata
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    dq isr_%+i
    %assign i i+1
%endrep
