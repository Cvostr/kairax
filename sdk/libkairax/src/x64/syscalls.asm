%macro DEFINE_SYSCALL 2
    global %1
    %1:
        mov rax, %2
        mov r10, rcx
        syscall
        ret
%endmacro

DEFINE_SYSCALL syscall_netctl,          0x304
DEFINE_SYSCALL syscall_routectl,        0x305
DEFINE_SYSCALL syscall_sysinfo,         0x306
DEFINE_SYSCALL syscall_netstat,         0x307
DEFINE_SYSCALL syscall_load_module,     0xAF
DEFINE_SYSCALL syscall_unload_module,   0xB0