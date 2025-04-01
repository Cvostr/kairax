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