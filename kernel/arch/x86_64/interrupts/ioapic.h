#ifndef _IOAPIC_H
#define _IOAPIC_H

#include "types.h"

uint32_t ioapic_read(uint32_t reg);

void ioapic_write(uint32_t reg, uint32_t value);

// IOAPIC существует в системе
int ioapic_is_available();

// Для локального APIC перенаправить vector на IRQ 
void ioapic_redirect_interrupt(int lapic_id, int vector, int irq);

// Для локального APIC перенаправить vector прерывания на общий GSI
void ioapic_redirect_interrupt_to_gsi(int lapic_id, int vector, int gsi, int iso_flags);

#endif