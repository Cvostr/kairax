bits 64

%include "memory/hh_offset.asm"

global init_x64
extern kernel_stack_top
extern gdtptr_hh
extern kmain

section .text

[section .text] 

init_x64:
    mov ax, 0x10            ; GDT_start.KernelData
    mov ds, ax              ; data segment
    mov es, ax              ; extra segment
    mov fs, ax              ; F-segment
    mov gs, ax              ; G-segment
    mov ss, ax              ; stack segment

    mov ebx, V2P(gdtptr_hh)
    lgdt [ebx]
    mov rsp, kernel_stack_top
    lea rax, [kmain]
    call rax