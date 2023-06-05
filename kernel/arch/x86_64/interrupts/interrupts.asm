%include "memory/mem_layout.asm"

[BITS 64]
[SECTION .text]

section .text
global idt_update ;функция загрузки таблицыы дескрипторов прерываний
idt_update:
	lidt  [rdi]
    ret

section .bss
global idt_descriptors ;таблица дескрипторов прерываний
idt_descriptors:
	resb 4096


extern int_handler ;Обработчик прерываний
extern scheduler_handler ;Обработчик прерываний

;Сегмент данных ядра
%define KERNEL_DATA_SEG 0x10

; Макросы для помещения всех регистров в стек и извлечения их оттуда
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


section .text
isr_stub:
    ; Поместить все 64-битные регистры в стек
    pushaq
    ; Push all the data segments (Stack alignment is kept because all segments add up to 64-bits here)
    mov ax, ds
    push ax
    mov ax, es
    push ax
    mov ax, fs
    push ax
    mov ax, gs
    push ax
    ; Load the kernel data segments
    mov ax, KERNEL_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ; And we get into it
    mov rdi, rsp
    cld
    call int_handler
	;nop
    ; Pop all the data segments
    pop ax
    mov ax, gs
    pop ax
    mov ax, fs
    pop ax
    mov ax, es
    pop ax
    mov ax, ds
    ; And pop all the registers
    popaq
    add rsp, 16 ; Pops the error number off the stack
    iretq

section .text
isr_stub_sc:
    ; Поместить все 64-битные регистры в стек
    pushaq
    ; Push all the data segments (Stack alignment is kept because all segments add up to 64-bits here)
    mov ax, ds
    push ax
    mov ax, es
    push ax
    mov ax, fs
    push ax
    mov ax, gs
    push ax
    ; Load the kernel data segments
    mov ax, KERNEL_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; And we get into it
    mov rdi, rsp
    cld
    call scheduler_handler

    ; Pop all the data segments
    pop ax
    mov ax, gs
    pop ax
    mov ax, fs
    pop ax
    mov ax, es
    pop ax
    mov ax, ds
    ; And pop all the registers
    popaq
    iretq

; We'll start by making all of our ISRs. From 0 to 255

; Make an ISR wrapper for each table
; Arg 1: ISR number
%macro isr_wrapper 1
isr_%1:
    ; Turn off interrupts
    cli
    ; Check if we have an error number with this ISR
    ; The ones with error numbers are:
    ; 8 - DF
    ; 10 -TS
    ; 11 - NP
    ; 12 - SS
    ; 13 - GP
    ; 14 - PF
    ; 17 - AC
    ; 21 - CP
    %ifn (%1 == 8) || ((%1 >= 10 ) && (%1 <= 14)) || (%1 == 17) || (%1 == 21)
        ; Push an error number of 0
        push 0
    %endif
    ; Push the interrupt number
    push %1
    jmp isr_stub ; And go to the isr_stub
%endmacro

isr_32:
    cli
    jmp isr_stub_sc ; And go to the isr_stub

; Create all 256 interrupt vectors.
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
