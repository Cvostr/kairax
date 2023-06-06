bits 64

%include "memory/mem_layout.asm"

global init_x64
global lgdt_hh
global write_cr3
global gdt_update
extern kernel_stack_top
extern gdtptr_hh
extern kmain

[section .text] 

lgdt_boot_v:
    push rbx
    mov rbx, gdtptr_hh
    lgdt [rbx]
    pop rbx
    ret

init_x64:
    mov ax, 0x10            ; GDT_start.KernelData
    mov ds, ax              ; data segment
    mov es, ax              ; extra segment
    mov fs, ax              ; F-segment
    mov gs, ax              ; G-segment
    mov ss, ax              ; stack segment

    mov rsp, kernel_stack_top
    call lgdt_boot_v
    lea rax, [kmain]
    call rax

write_cr3:
    mov cr3, rdi
    ret

gdt_update:
	lgdt  [rdi]
    ret