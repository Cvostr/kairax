bits 64

global boot_tss
global tss_size
global interrupt_stack_table

[section .data]
align 16
boot_tss:
    dd 0x0 ; Reseved 1
tss_rspn:
    dq 0x0 ; RSP0
    dq 0x0 ; RSP1
    dq 0x0 ; RSP2
    dq 0x0 ; Reserved
interrupt_stack_table:
    dq 0x0 ; IST1
    dq 0x0 ; IST2
    dq 0x0 ; IST3
    dq 0x0 ; IST4
    dq 0x0 ; IST5
    dq 0x0 ; IST6
    dq 0x0 ; IST7
    dq 0x0 ; Reserved
    dw 0x0 ; Reserved
    dw tss_size ; I/O Map Base Address
tss_size: equ $ - boot_tss - 1
