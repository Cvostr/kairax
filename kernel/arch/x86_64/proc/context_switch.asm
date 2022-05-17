global switch_context

section .text

switch_context:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov r15, QWORD [rdi + 0x20]
    mov r14, QWORD [rdi + 0x28]
    mov r13, QWORD [rdi + 0x30]
    mov r12, QWORD [rdi + 0x38]
    mov r11, QWORD [rdi + 0x40]
    mov r10, QWORD [rdi + 0x48]
    mov r9,  QWORD [rdi + 0x50]
    mov r8,  QWORD [rdi + 0x58]

    mov rcx, QWORD [rdi + 0x68]
    mov rdx, QWORD [rdi + 0x70]
    mov rbx, QWORD [rdi + 0x78]
    mov rsi, QWORD [rdi + 0x80]


    ; DATA SELECTOR
    push QWORD KERNEL_DATA_SELECTOR
    ; RSP
    mov rax, QWORD [rdi + 0xC0]
    push rax
    ; rflags
    mov rax, QWORD [rdi + 0xB8]
    push rax
    popfq
    pushfq

    push QWORD 0x8
    ; RIP
    mov rax, QWORD [rdi + 0xA8]
    push rax

    ; RBP
    mov rbp, QWORD [rdi + 0x90]

    ; RAX/RDI
    mov rax, QWORD [rdi + 0x60]
    mov rdi, QWORD [rdi + 0x88]
    iretq
