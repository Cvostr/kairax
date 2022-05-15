#ifndef _INTS_HANDLER
#define _INTS_HANDLER

#include "stdint.h"

typedef struct PACKED {
    // Our segment selectors
    uint16_t gs; /*!< GS segment selector */
    uint16_t fs; /*!< FS segment selector */
    uint16_t es; /*!< ES segment selector */
    uint16_t ds; /*!< DS segment selector */
    // The registers
    uint64_t r15; /*!< r15 register */
    uint64_t r14; /*!< r14 register */
    uint64_t r13; /*!< r13 register */
    uint64_t r12; /*!< r12 register */
    uint64_t r11; /*!< r11 register */
    uint64_t r10; /*!< r10 register */
    uint64_t r9;  /*!< r9 register */
    uint64_t r8;  /*!< r8 register */
    
    uint64_t rdi; /*!< RDI register */
    uint64_t rsi; /*!< RSI register */
    uint64_t rbp; /*!< RBP register */
    uint64_t rbx; /*!< RBX register */
    uint64_t rdx; /*!< RDX register */
    uint64_t rcx; /*!< RCX register */
    uint64_t rax; /*!< RAX register */

    uint64_t int_no; /*!< The number of the interrupt that was called */

    uint64_t error_code; /*!< The optional error code that was put on the stack by the interrupt */

    uint64_t rip; /*!< The program register prior to the interrupt call */
    uint64_t cs; /*!< The code segment prior to the interrupt call */
    uint64_t rflags; /*!< The rflags register prior to the interrupt call */
    uint64_t userrsp; /*!< The stack pointer prior to the interrupt call */
    uint64_t ss; /*!< The stack segment selctor prior to the interrupt call */
} interrupt_frame_t;


typedef void (*isr_handler_t)(interrupt_frame_t*);

void init_interrupts_handler();

void register_interrupt_handler(int interrupt_num, void* handler_func);

#endif
