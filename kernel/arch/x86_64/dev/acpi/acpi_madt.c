#include "acpi.h"
#include "memory/kernel_vmm.h"
#include "mem/pmm.h"
#include "stdio.h"
#include "kairax/kstdlib.h"

acpi_madt_t* acpi_madt_table;

// local APIC
#define MAX_CPU_COUNT 64
uint32_t            cpus_apic_count = 0;
apic_local_cpu_t*   cpus_apic[MAX_CPU_COUNT];

// IOAPIC ISO Override
#define MAX_IOAPIC_ISO_COUNT 64
uint32_t            ioapic_isos_count = 0;
ioapic_iso_t*       ioapic_isos[MAX_IOAPIC_ISO_COUNT];

// Global
apic_io_t*          ioapic_global;

uint32_t acpi_get_cpus_apic_count()
{
    return cpus_apic_count;
}

apic_local_cpu_t** acpi_get_cpus_apic()
{
    return cpus_apic;
}

apic_io_t*  acpi_get_global_apic()
{
    return ioapic_global;
}

acpi_madt_t* acpi_get_madt()
{
    return acpi_madt_table;
}

void acpi_parse_apic_madt(acpi_madt_t* madt)
{
    acpi_madt_table = (acpi_madt_t*)P2V(madt);

    uint8_t *p = (uint8_t *)(acpi_madt_table + 1);
    uint8_t *end = (uint8_t *)acpi_madt_table + acpi_madt_table->header.length;

    while (p < end) {
        apic_header_t* apic_header = (apic_header_t*)p;
        uint8_t type = apic_header->type;
        
        switch (type)
        {
        case 0:
            //Это локальный контроллер для одного из ядер процессора
            apic_local_cpu_t* cpu_apic = (apic_local_cpu_t*)p;

            if (cpu_apic->flags & 1) {
                cpus_apic[cpus_apic_count++] = cpu_apic;
            }

            break;
        case 1:
            // Это глобальный IOAPIC
            ioapic_global = (apic_io_t*)p;
            break;
        case 2:
            // IO APIC ISO Override
            ioapic_iso_t* ioapic_iso_override = (ioapic_iso_t*)p;
            ioapic_isos[ioapic_isos_count++] = ioapic_iso_override;
            break;
        case 3:
            lapic_nmi_source_t* lapic_nmi_src = (lapic_nmi_source_t*)p;
            // todo : сохранить структуру
            break;
        case 5:
            // Если такая структура есть - надо использовать адрес lapic из неё
            printk("64 bit APIC override");
            apic_override_t* ovstruct = (apic_override_t*)p;
        
        default:
            break;
        }

        p += apic_header->length;   
    }
}

ioapic_iso_t* acpi_madt_get_iso(uint32_t index)
{
    if (index < ioapic_isos_count) {
        return ioapic_isos[index];
    }

    return NULL;
}