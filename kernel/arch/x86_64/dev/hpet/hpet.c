#include "hpet.h"
#include "memory/paging.h"
#include "mem/pmm.h"
#include "memory/kernel_vmm.h"

uint64_t hpet_period;
uint64_t hpet_address;

void hpet_write(uint64_t reg, uint64_t val);
uint64_t hpet_read(uint64_t reg);

int init_hpet(acpi_hpet_t* hpet)
{
    hpet_address = (uint64_t) P2V(hpet->address);

    map_page_mem(get_kernel_pml4(),
			hpet_address,
			hpet->address,
			PAGE_PRESENT | PAGE_WRITABLE);  

    hpet_period = hpet_read(HPET_GENERAL_CAPABILITIES) >> HPET_COUNTER_CLOCK_OFFSET;

    //printk("HPET period %i\n", hpet_period);

    hpet_write(HPET_GENERAL_CONFIG, HPET_CONFIG_DISABLE);
    hpet_write(HPET_MAIN_COUNTER_VALUE, 0);
    hpet_write(HPET_GENERAL_CONFIG, HPET_CONFIG_ENABLE);
}

uint64_t hpet_get_counter()
{
    return hpet_read(HPET_MAIN_COUNTER_VALUE);
}

void hpet_reset_counter()
{
    hpet_write(HPET_GENERAL_CONFIG, HPET_CONFIG_DISABLE);
    hpet_write(HPET_MAIN_COUNTER_VALUE, 0);
    hpet_write(HPET_GENERAL_CONFIG, HPET_CONFIG_ENABLE);
}

void hpet_sleep(uint64_t milliseconds)
{
    uint64_t target = hpet_read(HPET_MAIN_COUNTER_VALUE) + (milliseconds * 1000000000000) / hpet_period;
    while (!(hpet_read(HPET_MAIN_COUNTER_VALUE) >= target))
    {
        asm volatile("pause");
    }
}

void hpet_nanosleep(uint64_t nanoseconds)
{
    uint64_t target = hpet_read(HPET_MAIN_COUNTER_VALUE) + (nanoseconds * 1000000) / hpet_period;
    while (hpet_read(HPET_MAIN_COUNTER_VALUE) < target)
    {
        asm volatile("pause");
    }
}

void hpet_write(uint64_t reg, uint64_t val)
{
	*((volatile uint64_t*)(hpet_address + reg)) = val;
}

uint64_t hpet_read(uint64_t reg)
{
	return *((volatile uint64_t*)(hpet_address + reg));
}