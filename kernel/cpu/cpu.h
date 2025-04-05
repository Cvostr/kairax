#ifndef _CPU_H
#define _CPU_H

#include "types.h"

#define CPU_VENDOR_STR_LEN 20
#define CPU_MODEL_STR_LEN 50

// CPU архитектуры
#define CPU_ARCH_X86_64     1
#define CPU_ARCH_AARCH64    2
#define CPU_ARCH_RISCV      3

// CPU порядки байт
#define CPU_BYTEORDER_BE    1
#define CPU_BYTEORDER_LE    2

struct cpu {
    uint16_t arch;
    uint8_t byteorder;
    uint8_t sockets;
    uint16_t cpus;
    char vendor_string[CPU_VENDOR_STR_LEN];
    char model_string[CPU_MODEL_STR_LEN];
};

int cpu_get_info(struct cpu* cpuinf);

#endif