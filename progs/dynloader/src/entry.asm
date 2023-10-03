[BITS 64]
default rel
global linker_entry
global _start
extern link
extern loader
extern aux_vector
extern args_info

linker_entry:
    push rdi
    mov rdi, [rsp + 8]
    push rsi
    mov rsi, [rsp + 24]
    push rcx
    push rdx
    push rbx

    lea rax, [rel link]
    call rax

    pop rbx
    pop rdx
    pop rcx
    pop rsi
    pop rdi

    add rsp, 16

    jmp rax

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

    ; Извлечение argv
    pop rax
    mov [rcx + 8], rax

    ; Извлечение argc
    pop rax
    mov [rcx + 0], rax

    jmp loader