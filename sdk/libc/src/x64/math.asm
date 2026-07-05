[BITS 64]
[SECTION .text]

global sin
sin:
    push rbp
    mov rbp, rsp
    movsd [rbp - 0x8], xmm0

    fld qword [rbp - 0x08] 
    fsin
    fstp qword [rbp - 0x8]

    movsd xmm0, [rbp - 0x8]
    pop rbp
    ret

global cos
cos:
    push rbp
    mov rbp, rsp
    movsd [rbp - 0x8], xmm0

    fld qword [rbp - 0x08] 
    fcos
    fstp qword [rbp - 0x8]

    movsd xmm0, [rbp - 0x8]
    pop rbp
    ret

global tan
tan:
    push rbp
    mov rbp, rsp
    movsd [rbp - 0x8], xmm0

    fld qword [rbp - 0x08]  ; ST(0)
    fptan
    fstp qword [rbp - 0x8] ; ST(0)
    fstp qword [rbp - 0x10] ; ST(1)

    movsd xmm0, [rbp - 0x10]
    pop rbp
    ret

global sqrt
sqrt:
    push rbp
    mov rbp, rsp
    movsd [rbp - 0x8], xmm0

    fld qword [rbp - 0x08] 
    fsqrt
    fstp qword [rbp - 0x8]

    movsd xmm0, [rbp - 0x8]
    pop rbp
    ret

global atan2
atan2:
    push rbp
    mov rbp, rsp
    movsd [rbp - 0x8], xmm0
    movsd [rbp - 0x10], xmm1

    fld qword [rbp - 0x08] 
    fld qword [rbp - 0x10]
    fpatan
    fstp qword [rbp - 0x8]

    movsd xmm0, [rbp - 0x8]
    pop rbp
    ret

global pow
pow:
    push rbp
    mov rbp, rsp
    movsd [rbp - 0x8], xmm0
    movsd [rbp - 0x10], xmm1

    fld qword [rbp - 0x10] ; ST(0)
    fld qword [rbp - 0x8] ; ST(1)
    fyl2x  ; 2^(y*log2(x))
    fld1
    fld st1
    fprem
    f2xm1
    faddp st1, st0
    fscale
    fstp qword [rbp - 0x8]
    fstp qword [rbp - 0x10]

    movsd xmm0, [rbp - 0x8]
    pop rbp
    ret

global ldexp
ldexp:
    push rbp
    mov rbp, rsp
    ; копирование аргументов в ST(0), ST(1)
    movsd [rbp - 0x8], xmm0
    mov [rbp - 0xC], edi
    fild dword [rbp - 0xC]
    fld qword [rbp - 0x8]
    ; ST(0) = ST(0) * 2 ^(ST(1))
	fscale
    ; ST(0) -> xmm0
    fstp qword [rbp - 0x8]
    movsd xmm0, [rbp - 0x8]

    pop rbp
	ret

global frexp
frexp:
    push rbp
    mov rbp, rsp
    ; xmm0 -> ST(0)
    movsd [rbp - 0x8], xmm0
    fld qword [rbp - 0x8]
    ; m -> ST(0), exp -> ST(1)
    fxtract
    ; ST(0) -> xmm0
    fstp qword [rbp - 0x8]
    movsd xmm0, [rbp - 0x8]
    ; делим xmm0 на 2, чтобы соответствовать стандарту frexp
    mov rax, 0x3fe0000000000000
    movq xmm1, rax 
    mulsd xmm0, xmm1
    ; передаем экспоненту с увеличением на 1
    fistp dword [rdi]
    inc [rdi]

    pop rbp
    ret 


global fmod
fmod:
    push rbp
    mov rbp, rsp
    movsd [rbp - 0x8], xmm0
    movsd [rbp - 0x10], xmm1

    fld qword [rbp - 0x10] ; ST(0)
    fld qword [rbp - 0x8] ; ST(1)
.repeat:
    fprem
    fnstsw ax
    test ah, 100b
    jnz .repeat

    fstp st1
    fstp qword [rbp - 0x8] ; ST(0)

    movsd xmm0, [rbp - 0x8]
    pop rbp
    ret

global log
global logf
global log2
global log2f
global log10
global log10f
logf:
    push rbp
    mov rbp, rsp
    fldln2
    jmp __logf

log:
    push rbp
    mov rbp, rsp
    fldln2
    jmp __log

log2f:
    push rbp
    mov rbp, rsp
    fld1
    jmp __logf

log2:
    push rbp
    mov rbp, rsp
    fld1
    jmp __log

log10:
    push rbp
    mov rbp, rsp
    fldlg2
    jmp __log

log10f:
    push rbp
    mov rbp, rsp
    fldlg2
    jmp __logf

__log:
    ; xmm0 -> ST(0)
    movsd [rbp - 0x8], xmm0
    fld qword [rbp - 0x8]
    ; 
    fyl2x
    ; ST(0) -> xmm0
    fstp qword [rbp - 0x8]
    movsd xmm0, [rbp - 0x8]

    pop rbp
	ret

__logf:
    ; xmm0 -> ST(0)
    movss [rbp - 0x4], xmm0
    fld dword [rbp - 0x4]
    ; 
    fyl2x
    ; ST(0) -> xmm0
    fstp dword [rbp - 0x4]
    movss xmm0, [rbp - 0x4]

    pop rbp
	ret