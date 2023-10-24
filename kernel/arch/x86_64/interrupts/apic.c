#include "apic.h"
#include "dev/acpi/acpi.h"
#include "memory/mem_layout.h"
#include "cpu/msr.h"
#include "stdio.h"
#include "kstdlib.h"

void* local_apic_addr = NULL;   // Общий адрес для всех ядер

int apic_init()
{
    local_apic_addr = P2V(acpi_get_madt()->local_apic_address);

    //uint64_t vv;
    //cpu_msr_get(MSR_APIC_BASE, &vv);
    //cpu_msr_set(MSR_APIC_BASE, vv);
    //printf("APIC = %s \n", ulltoa(vv, 16));

    // Инициализация таймера PIT
    lapic_write(LAPIC_REG_LVT_TIMER, (1 << 16) | 0xff);
    lapic_write(LAPIC_REG_TIMER_DIV, 0);

    // Сбрость PIT
    //pit_set_reload_value(0xffff);

    //uint16_t initial_tick = pit_get_count();
    //uint64_t samples = 0xfffff;

    //lapic_write(LAPIC_REG_TIMER_INITCNT, (uint32_t)samples);
    //while (lapic_read(LAPIC_REG_TIMER_CURCNT) != 0);

    lapic_write(LAPIC_REG_SPURIOUS, lapic_read(LAPIC_REG_SPURIOUS) | 0x100 | 0xFF);
    
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

void lapic_timer_stop()
{
    lapic_write(LAPIC_REG_TIMER_INITCNT, 0);
    lapic_write(LAPIC_REG_LVT_TIMER, 1 << 16);
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