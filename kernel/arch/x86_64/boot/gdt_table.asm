%include "memory/mem_layout.asm"

global gdtptr
global gdtptr_hh
extern tss_size
section .rodata
gdtptr:
    dw gdt_end - gdt_start - 0x1
    dd K2P(gdt)

gdtptr_hh:
    dw gdt_end - gdt_start - 0x1
    dq gdt

section .data
align 16
gdt:
gdt_start:
    ; NULL SELECTOR
    dd 0x0
    dd 0x0
;Kernel code дескриптор (Смещение 0x8)
gdt_kernel_code: equ $ - gdt_start
    dw 0xFFFF                    ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 10011010b                 ; Access (exec/read).
    db 00101111b                 ; Granularity, 64 bits flag, limit19:16.
    db 0                         ; Base (high).
;Kernel data дескриптор (Смещение 0x10)
gdt_kernel_data: equ $ - gdt_start
    dw 0xFFFF                    ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 10010010b                 ; Access (read/write).
    db 00100000b                 ; Granularity.
    db 0                         ; Base (high).

gdt_user_code: equ $ - gdt_start   ; The User code descriptor.
    dw 0xFFFF                    ; Limit (low).
    dw 0x0                       ; Base (low).
    db 0x0                       ; Base (middle)
    db 11111010b                 ; Access (exec/read).
    db 00100000b                 ; Granularity, 64 bits flag, limit19:16.
    db 0                         ; Base (high).

gdt_user_data: equ $ - gdt_start   ; The user data descriptor.
    dw 0xFFFF                    ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 11110010b                 ; Access (read/write).
    db 00100000b                 ; Granularity.
    db 0     

gdt_tss: equ $ - gdt_start
    dw tss_size                 ; Limit
	dw 0x0                      ; Base (bytes 0-2)
	db 0x0                      ; Base (byte 3)
	db 10001001b                ; Type, present
	db 10000000b                ; Misc
	db 0x0                      ; Base (byte 4)
	dd 0x0                      ; Base (bytes 5-8)
	dd 0x0                      ; Zero / reserved

    ; NULL SELECTOR
    dd 0x0
    dd 0x0
gdt_end:
gdt_size: equ gdt_end - gdt_start