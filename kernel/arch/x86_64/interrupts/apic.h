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

#define LAPIC_TIMER_PERIODIC        0x20000
#define LAPIC_TIMER_MASKED          0x10000

#define IPI_TYPE_FIXED  0
#define IPI_TYPE_LOW_PRIORITY   (1 << 8)
#define IPI_TYPE_INIT           (5 << 8)
#define IPI_TYPE_STARTUP        (6 << 8)

#define IPI_DST_BY_ID           0
#define IPI_DST_SELF            (1 << 18)
#define IPI_DST_ALL             (2 << 18)
#define IPI_DST_OTHERS          (3 << 18)

int apic_init();

int lapic_timer_calibrate(int freq);

void lapic_write(uint32_t reg, uint32_t val);

uint32_t lapic_read(uint32_t reg);

void lapic_send_ipi(uint32_t lapic_id, uint32_t dst, uint32_t type, uint8_t vector);

void lapic_eoi();

#endif