#ifndef _APIC_H
#define _APIC_H

#include "types.h"

#define LAPIC_REG_ID                0x20 
#define LAPIC_REG_EOI               0x0b0
#define LAPIC_REG_SPURIOUS          0x0f0
#define LAPIC_REG_CMCI              0x2f0
#define LAPIC_REG_ICR0              0x300
#define LAPIC_REG_ICR1              0x310
#define LAPIC_REG_LVT_TIMER         0x320
#define LAPIC_REG_TIMER_INITCNT     0x380
#define LAPIC_REG_TIMER_CURCNT      0x390
#define LAPIC_REG_TIMER_DIV         0x3e0
#define LAPIC_EOI_ACK               0x00

int apic_init();

void lapic_write(uint32_t reg, uint32_t val);

uint32_t lapic_read(uint32_t reg);

void lapic_timer_stop();

void lapic_send_ipi(uint32_t lapic_id, uint32_t value);

void lapic_eoi();

#endif