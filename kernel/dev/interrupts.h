#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include "kairax/types.h"

#define INT_TYPE_X86_IOAPIC 2
#define INT_TYPE_X86_MSI 3
#define INT_TYPE_X86_MSIX 4

struct interrupt {
    uint16_t irq;
    int type;
    const char* desc;
};

int alloc_irq(int type, const char* desc);

int free_irq(int irq);

int register_irq_handler(int irq, void* handler, void* data);

//todo : опознавание устройства
int unregister_irq_handler(int irq);

#endif