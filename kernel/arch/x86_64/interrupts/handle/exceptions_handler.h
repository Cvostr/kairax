#ifndef _EXCEPTIONS_HANDLER_H
#define _EXCEPTIONS_HANDLER_H

#include "handler.h"

enum x86_exceptions_ints {
    EXCEPTION_DIVISION_BY_ZERO    = 0x0,
    EXCEPTION_DEBUG               = 0x1,
    EXCEPTION_NON_MASKABLE_INT    = 0x2,
    EXCEPTION_BREAKPOINT          = 0x3,
    EXCEPTION_OVERFLOW            = 0x4,
    EXCEPTION_OUT_OF_BOUNDS       = 0x5,
    EXCEPTION_INVALID_OPCODE      = 0x6,
    EXCEPTION_DEVICE_NA           = 0x7,
    EXCEPTION_DOUBLE_FAULT        = 0x8,
    EXCEPTION_COCPU_SEG_OVERRUN   = 0x9,
    EXCEPTION_INVALID_TSS         = 0xA,
    EXCEPTION_SEGMENT_NOT_PRESENT = 0xB,
    EXCEPTION_STACK_FAULT         = 0xC,
    EXCEPTION_GP_FAULT            = 0xD,
    EXCEPTION_PAGE_FAULT          = 0xE,
    EXCEPTION_UNKNOW_INT          = 0xF,
    EXCEPTION_FPU_FAULT           = 0x10,
    EXCEPTION_ALIGNMENT_CHECK     = 0x11,
    EXCEPTION_MACHINE_CHECK       = 0x12,
    EXCEPTION_SIMD                = 0x13,
    EXCEPTION_VIRT                = 0x14,
    EXCEPTION_SECURITY            = 0x1E,
};

void exception_handler(interrupt_frame_t* frame);

#endif
