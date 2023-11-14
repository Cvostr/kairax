bits 64

%include "memory/mem_layout.asm"

global init_x64
global lgdt_hh
global write_cr3
global read_cr3
global gdt_update
extern kernel_stack_top
extern gdtptr_hh
extern kmain
extern x64_ltr

[section .text] 

lgdt_boot_v:
    push rdi
    mov rdi, gdtptr_hh
    call gdt_update
    pop rdi
    ret

init_x64:
    mov ax, 0x10            ; Сегмент данных ядра
    mov ds, ax              
    mov es, ax              
    mov fs, ax              
    mov gs, ax              
    mov ss, ax              

    mov rsp, P2V(K2P(kernel_stack_top))
    call lgdt_boot_v
    lea rax, [kmain]
    call rax

write_cr3:
    mov cr3, rdi
    ret

read_cr3:
    mov rax, cr3
    ret

gdt_update:
	lgdt  [rdi]
    push 0x08                 
    lea rax, [rel .reload_code_seg]
    push rax                  
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

global x64_lidt ;функция загрузки таблицыы дескрипторов прерываний
x64_lidt:
	lidt  [rdi]
    ret

global enable_interrupts 
enable_interrupts:
    sti
    ret

global disable_interrupts 
disable_interrupts:
    cli
    ret