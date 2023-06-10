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

    uint16_t gs; 
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

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp; 
    uint64_t ss;
} thread_frame_t;

typedef struct PACKED {
    char            name[32];
    uint64_t        thread_id;
    void*           stack_ptr;
    int             state;
    process_t*      process;
    thread_frame_t  context;
    int is_userspace;
} thread_t;

thread_t* new_thread(process_t* process);

thread_t* create_kthread(process_t* process, void (*function)(void));

thread_t* create_thread(process_t* process, uintptr_t entry);

#endif