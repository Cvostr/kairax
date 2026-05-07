[BITS 64]
[SECTION .text]

extern main
global _start

extern __fflush_atexit
extern __atexit_callall

_start:
    ; Записать значение envp (приходит в rdx) в __environ
    mov [__environ], rdx
    ; Сохранить значения регистров-аргументов перед вызовом функции подготовки 
    push rdi
    push rsi
    push rdx

    ; Подготовить libc к работе
    call __fflush_atexit wrt ..plt

    ; Восстановить значения регистров-аргументов
    pop rdx
    pop rsi
    pop rdi
    ; Наша программа
    call main
    ; Сохранить в стеке возвращаемое значение
    push rax

    ; Выполнить все atexit()
    call __atexit_callall wrt ..plt

    ; Восстановить возвращаемое значение из стека как первый аргумент
    pop rdi
    ; Завершить процесс
    mov rax, 0x3C
    syscall

section .data
__environ: dq 0
global __environ