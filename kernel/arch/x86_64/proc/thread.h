#ifndef _THREAD_H
#define _THREAD_H

#include "stdint.h"
#include "process.h"

#include "interrupts/handle/handler.h"

#define STACK_SIZE 4096

enum thread_state {
    THREAD_RUNNING          = 0,
    THREAD_INTERRUPTIBLE    = 1,
    THREAD_UNINTERRUPTIBLE  = 2,
    THREAD_ZOMBIE           = 4,
    THREAD_STOPPED          = 8,
    THREAD_SWAPPING         = 16,
    THREAD_EXCLUSIVE        = 32,
    THREAD_CREATED          = 64,
    THREAD_LOADING          = 128
};

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

    uint64_t rip; /*!< The program register prior to the interrupt call */
    uint64_t cs; /*!< The code segment prior to the interrupt call */
    uint64_t rflags; /*!< The rflags register prior to the interrupt call */
    uint64_t rsp; /*!< The stack pointer prior to the interrupt call */
    uint64_t ss; /*!< The stack segment selctor prior to the interrupt call */
} thread_frame_t;

typedef struct PACKED {
    char            name[32];
    uint64_t        thread_id;
    void*           stack_ptr;
    int             state;
    process_t*      process;
    thread_frame_t  context;
} thread_t;

thread_t* create_new_thread(process_t* process, void (*function)(void));

#endif