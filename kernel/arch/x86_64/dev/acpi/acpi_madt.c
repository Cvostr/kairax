#include "acpi.h"
#include "memory/kernel_vmm.h"
#include "mem/pmm.h"

acpi_madt_t* acpi_madt_table;

#define MAX_CPU_COUNT 64
uint32_t            cpus_apic_count = 0;
apic_local_cpu_t*   cpus_apic[MAX_CPU_COUNT];

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
        case 5:
            // Если такая структура есть - надо использовать адрес lapic из неё
            printf("64 bit APIC override");
            apic_override_t* ovstruct = (apic_override_t*)p;
            madt->local_apic_address = ovstruct->addr;
        
        default:
            break;
        }

        p += apic_header->length;   
    }
}
