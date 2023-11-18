#include "ioapic.h"
#include "dev/acpi/acpi.h"
#include "memory/mem_layout.h"

extern apic_io_t*          ioapic_global;

extern uint32_t            ioapic_isos_count = 0;
extern ioapic_iso_t**      ioapic_isos;

uint32_t ioapic_read(uint32_t reg)
{
    uint32_t* addr = (uint32_t*) P2V(ioapic_global->io_apic_address);
    *addr = (reg & 0xFF);
    return addr[4];
}

void ioapic_write(uint32_t reg, uint32_t value)
{
    uint32_t* addr = (uint32_t*) P2V(ioapic_global->io_apic_address);
    *addr = (reg & 0xff);
    addr[4] = value;
}