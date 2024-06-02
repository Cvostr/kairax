#include "apic.h"
#include "dev/acpi/acpi.h"
#include "memory/mem_layout.h"
#include "cpu/msr.h"
#include "stdio.h"
#include "kstdlib.h"
#include "dev/cmos/cmos.h"
#include "dev/hpet/hpet.h"
#include "kairax/intctl.h"

#define NANOSECONDS_PER_SECOND 1000000000
#define LAPIC_TIMER_IRQ 0x20

void* local_apic_addr = NULL;   // Общий адрес для всех ядер

int apic_init()
{
    if (acpi_get_madt() == NULL) {
        printk("ERROR: apic_init(): MADT table is not present!\n");
        return -1;
    }

    local_apic_addr = P2V(acpi_get_madt()->local_apic_address);

    if (local_apic_addr == NULL) {
        printk("ERROR: apic_init(): LAPIC address is not present!\n");
        return -1;
    }

    lapic_write(LAPIC_REG_SPURIOUS, lapic_read(LAPIC_REG_SPURIOUS) | 0x100 | 0xFF);

    return 0;
}

void lapic_write(uint32_t reg, uint32_t val)
{
	*((volatile uint32_t*)(local_apic_addr + reg)) = val;
	asm volatile ("":::"memory");
}

uint32_t lapic_read(uint32_t reg)
{
	return *((volatile uint32_t*)(local_apic_addr + reg));
}

int lapic_timer_calibrate(int freq)
{
    int divisor = 0x3;
    if (!hpet_is_available()) {
        printk("ERROR: lapic_timer_calibrate() : HPET is not available!\n");
        return -1;
    }

    lapic_write(LAPIC_REG_TIMER_DIV, divisor);         // set divisor
    lapic_write(LAPIC_REG_TIMER_INITCNT, 0xFFFFFFFF);   // set timer counter
    
    hpet_nanosleep(NANOSECONDS_PER_SECOND / 1000);

    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_MASKED);

    uint32_t calibration = 0xFFFFFFFF - lapic_read(LAPIC_REG_TIMER_CURCNT);
    
    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_IRQ | LAPIC_TIMER_PERIODIC);
    lapic_write(LAPIC_REG_TIMER_DIV, divisor);
    lapic_write(LAPIC_REG_TIMER_INITCNT, calibration);

    return 0;
}

void lapic_send_ipi(uint32_t lapic_id, uint32_t value)
{
    lapic_write(0x310, lapic_id << 24);
	lapic_write(0x300, value);
	do { asm volatile ("pause" : : : "memory"); } while (lapic_read(0x300) & (1 << 12));
}

void lapic_eoi()
{
    lapic_write(LAPIC_REG_EOI, LAPIC_EOI_ACK);
}