[BITS 64]
default rel
global linker_entry
global _start
extern link
extern main
extern aux_vector

linker_entry:
    pop rax
    pop rbx

    push rdi
    push rsi
    push rcx

    mov rax, rdi
    mov rbx, rsi

    lea rcx, [rel link]
    call rcx

    pop rcx
    pop rsi
    pop rdi

    ret
    ;jmp rax

_start:
    lea rcx, [rel aux_vector]
    pop rax
    cmp rax, 0
    je enter
    pop rbx
    mov rbx, [rcx + 8 * rax]
    jmp _start

enter:
    jmp main