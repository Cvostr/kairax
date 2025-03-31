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