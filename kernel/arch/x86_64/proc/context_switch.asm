global switch_context

section .text

switch_context:
    ;Сохранение значений регистров предыдущего потока
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov [rdi], rsp   ;Адрес стека записывается в 1й аргумент
    mov rsp, [rsi]   ;Из второго аргумента берется адрес стека следующего потока

    ;Возвращение значений регистров
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ret ;Переход на адрес в стеке