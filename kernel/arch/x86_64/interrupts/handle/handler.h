#ifndef _INTS_HANDLER
#define _INTS_HANDLER

#include "types.h"

typedef struct PACKED {
    uint16_t fs; 
    uint16_t es; 
    uint16_t ds;
    
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;

    uint64_t int_no;

    uint64_t error_code;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} interrupt_frame_t;


typedef void (*isr_handler_t)(interrupt_frame_t*);

void init_interrupts_handler();

void register_interrupt_handler(int interrupt_num, void* handler_func);

#endif
