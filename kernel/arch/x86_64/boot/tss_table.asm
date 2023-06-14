bits 64

global boot_tss
global tss_size
global interrupt_stack_table

[section .data]
align 16
boot_tss:
    dd 0x0 ; Reseved 1
tss_rspn:
    dq rspn_stack ; RSP0
    dq rspn_stack ; RSP1
    dq rspn_stack ; RSP2
    dq 0x0 ; Reserved
interrupt_stack_table:
    dq istn_stack ; IST1
    dq istn_stack ; IST2
    dq istn_stack ; IST3
    dq istn_stack ; IST4
    dq istn_stack ; IST5
    dq istn_stack ; IST6
    dq istn_stack ; IST7
    dq 0x0 ; Reserved
    dw 0x0 ; Reserved
    dw tss_size ; I/O Map Base Address
tss_size: equ $ - boot_tss - 1

; btw, alloc two stack for that, we have a lot of memory :)
[section .bss]
istn_stack: RESB 0x2000
rspn_stack: RESB 0x2000
