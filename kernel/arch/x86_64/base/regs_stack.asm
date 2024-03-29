; Макросы для помещения всех регистров в стек и извлечения их оттуда
%macro pushaq 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro pushsg 0
    mov ax, ds
    push ax
    mov ax, es
    push ax
%endmacro

%macro popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

%macro popsg 0
    pop ax
    mov es, ax
    pop ax
    mov ds, ax
%endmacro