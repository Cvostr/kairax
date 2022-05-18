#ifndef _CPU_CONTEXT_H
#define _CPU_CONTEXT_H

#include "stdint.h"

typedef struct PACKED {
    uint64_t gs;      // 0x0
    uint64_t fs;      // 0x8
    uint64_t es;      // 0x10
    uint64_t ds;      // 0x18

    uint64_t r15;     // 0x20
    uint64_t r14;     // 0x28
    uint64_t r13;     // 0x30
    uint64_t r12;     // 0x38
    uint64_t r11;     // 0x40
    uint64_t r10;     // 0x48
    uint64_t r9;      // 0x50
    uint64_t r8;      // 0x58

    uint64_t rax;     // 0x60
    uint64_t rcx;     // 0x68
    uint64_t rdx;     // 0x70
    uint64_t rbx;     // 0x78
    uint64_t rsi;     // 0x80
    uint64_t rdi;     // 0x88

    uint64_t rbp;     // 0x90

    uint64_t UNUSED1; // 0x98
    uint64_t UNUSED2; // 0xA0

    uint64_t rip;     // 0xA8
    uint64_t cs;      // 0xB0
    uint64_t rflags;  // 0xB8
    uint64_t rsp;     // 0xC0
    uint64_t ss;      // 0xC8

    /* UNREACHABLE FOR THE MOMENT */
    uint64_t cr3;     // 0xD8

    // xmm0..xmm15, 128 bit registers
    // uint64 xmm[32]; // 0xE0
} cpu_context_t;

#endif