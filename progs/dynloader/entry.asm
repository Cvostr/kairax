[BITS 64]
default rel
global linker_entry
extern link

linker_entry:
    pop rax
    pop rbx

    push rdi
    push rsi
    push rcx

    mov rax, rdi
    mov rbx, rsi

    call link

    pop rcx
    pop rsi
    pop rdi

    jmp rax
