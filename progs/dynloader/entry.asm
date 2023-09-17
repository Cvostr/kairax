[BITS 64]
default rel
global linker_entry
global _start
extern link
extern main
extern aux_vector
extern args_info

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
    ; Этап извлечения aux вектора
    ; Получение относительного адреса aux вектора
    lea rcx, [rel aux_vector]

aux_loop:
    pop rax     ; Тип
    pop rbx     ; Значение
    
    mov [rcx + 8 * rax], rbx
    ; Если встретили AT_NULL, то выходим из цикла
    cmp rax, 0
    je enter

    jmp aux_loop

enter:

    ; Этап извлечения argc, argv
    ; Получение относительного адреса args массива
    lea rcx, [rel args_info]

    pop rax
    mov [rcx + 8], rax

    pop rax
    mov [rcx + 0], rax

    jmp main