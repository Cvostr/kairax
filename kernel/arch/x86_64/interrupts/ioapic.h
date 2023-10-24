#ifndef _IOAPIC_H
#define _IOAPIC_H

#include "types.h"

uint32_t ioapic_read(uint32_t reg);

void ioapic_write(uint32_t reg, uint32_t value);

#endif