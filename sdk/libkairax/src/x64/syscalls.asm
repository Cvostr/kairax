%macro DEFINE_SYSCALL 2
    global %1
    %1:
        mov rax, %2
        mov r10, rcx
        syscall
        ret
%endmacro

DEFINE_SYSCALL syscall_netctl, 0x304