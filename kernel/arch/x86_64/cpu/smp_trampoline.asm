%include "memory/mem_layout.asm"

; адреса начала и конца кода трамплина
global ap_trampoline
global ap_trampoline_end

global ap_gdtr
global ap_vmem

extern ap_init
extern ap_stack

bits 16
section .text
ap_trampoline:
    mov eax, 0xA0
    mov cr4, eax

    mov edx, [cs : ap_vmem - ap_trampoline]
    mov cr3, edx

    mov ecx, dword 0xC0000080
	rdmsr
	or eax, (1 << 8) | (0x1 << 0xB) | (1 << 11) | 1 ; Включение Long Mode, вызова syscall, SVME
	wrmsr

    ; Включение страничной адресации
	mov eax, cr0
	or eax, (1 << 31) | (1 << 0) | (1 << 16)
	mov cr0, eax
    
    o32 lgdt [cs : dword ap_gdtr - ap_trampoline]
    jmp dword 0x08:(0x1000 + ap_farjump - ap_trampoline)

bits 64
align 16
ap_farjump:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov gs, ax
    mov fs, ax
    mov es, ax
    ; на стартовом TSS не делаем LTR

    mov rsp, [ap_stack]
    mov rbp, rsp

    lea rax, [ap_init] 
    call QWORD rax

align 16 
ap_gdtr:
    dw 0
    dq 0
ap_vmem:
    dd 0

ap_trampoline_end: