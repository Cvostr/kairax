#include "ioapic.h"
#include "dev/acpi/acpi.h"
#include "memory/mem_layout.h"
#include "kairax/kstdlib.h"

extern apic_io_t*          ioapic_global;

extern uint32_t            ioapic_isos_count;
extern ioapic_iso_t**      ioapic_isos;

uint32_t ioapic_read(uint32_t reg)
{
    uint32_t volatile* addr = (uint32_t volatile*) P2V(ioapic_global->io_apic_address);
    *addr = (reg & 0xFF);
    return addr[4];
}

void ioapic_write(uint32_t reg, uint32_t value)
{
    uint32_t volatile* addr = (uint32_t volatile*) P2V(ioapic_global->io_apic_address);
    *addr = (reg & 0xff);
    addr[4] = value;
}

int ioapic_is_available() {
    return ioapic_global != NULL;
}

void ioapic_redirect_interrupt(int lapic_id, int vector, int irq)
{
    if (!ioapic_is_available()) {
        return;
    }

    // Сначала ищем в iso
    int iso_index = 0;
    ioapic_iso_t* iso;
    while ((iso = acpi_madt_get_iso(iso_index++)) != NULL) {

        if (iso->irq_source == irq) {
            // Для IRQ найдено переопределение ISO - используем его
            ioapic_redirect_interrupt_to_gsi(lapic_id, vector, iso->gsi, iso->flags);
            return;
        }
    }

    // Переопределения для этого IRQ нет - перенаправляем по приципу IRQ = GSI
    ioapic_redirect_interrupt_to_gsi(lapic_id, vector, irq, 0);
}

void ioapic_redirect_interrupt_to_gsi(int lapic_id, int vector, int gsi, int iso_flags)
{
    uint64_t value = vector & 0x7F;
    value |= (((uint64_t) lapic_id) << 56);

    // Обработка флагов IS Override
    if (iso_flags & 2) {
        // Active when LOW
        value |= (1 << 13);
    }

    if (iso_flags & 8) {
        // Level triggered
        value |= (1 << 15);
    }

    int redir_table_pos = 0x10 + (gsi - ioapic_global->global_sys_interrupt_base) * 2;
    ioapic_write(redir_table_pos, value & 0xFFFFFFFF);
    ioapic_write(redir_table_pos + 1, (value >> 32) & 0xFFFFFFFF);
}