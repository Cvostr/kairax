#include "stdint.h"
#include "stdio.h"

typedef struct PACKED 
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rax;

    // Задаются вызовом SYSCALL
    uint64_t rcx;
    uint64_t r11;
} syscall_frame_t;

void syscall_handle(syscall_frame_t* frame) {
    char* mem = (char*)frame->rdi;
    if (frame->rax == 1)
        printf("%s", mem);
}