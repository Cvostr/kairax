#include "pit.h"
#include "io.h"

#define PIT_DIVIDEND 1193182ULL

uint32_t pit_get_count()
{
    outb(0x43, 0);
	uint16_t counth = inb(0x40);
    uint8_t countl = inb(0x40);
	return counth << 8 | countl;
}

void pit_set_reload_value(uint32_t count)
{
    outb(0x43, 0x34);
    outb(0x40, (uint8_t)count);
    outb(0x40, (uint8_t)(count >> 8));
}

void pit_set_frequency(uint64_t frequency)
{
    uint64_t new_divisor = PIT_DIVIDEND / frequency;
    
    if (PIT_DIVIDEND % frequency > frequency / 2) {
        new_divisor++;
    }

    pit_set_reload_value((uint16_t)new_divisor);
}