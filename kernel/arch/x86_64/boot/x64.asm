bits 64

%include "memory/mem_layout.asm"

global init_x64
global lgdt_hh
global write_cr3
global gdt_update
extern kernel_stack_top
extern gdtptr_hh
extern kmain
extern x64_ltr

[section .text] 

lgdt_boot_v:
    push rbx
    mov rbx, gdtptr_hh
    lgdt [rbx]
    pop rbx
    ret

init_x64:
    mov ax, 0x10            ; Сегмент данных ядра
    mov ds, ax              
    mov es, ax              
    mov fs, ax              
    mov gs, ax              
    mov ss, ax              

    mov rsp, kernel_stack_top
    call lgdt_boot_v
    lea rax, [kmain]
    call rax

write_cr3:
    mov cr3, rdi
    ret

gdt_update:
	lgdt  [rdi]
    push 0x08                 
    lea rax, [rel .reload_code_seg]
    push RAX                  
    retfq                    

.reload_code_seg:
   MOV   AX, 0x10 ; Сегмент данных ядра
   MOV   DS, AX
   MOV   ES, AX
   MOV   FS, AX
   MOV   GS, AX
   MOV   SS, AX
   RET

x64_ltr:
    ltr di
    ret