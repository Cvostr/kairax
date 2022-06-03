#include "acpi.h"
#include "stdio.h"
#include "string.h"
#include "memory/paging.h"

#define to_acpi_header(x)  (acpi_header_t*)(uintptr_t)(x)

acpi_rsdp_t acpi_rsdp;

acpi_fadt_t* acpi_fadt;
acpi_madt_t* acpi_apic;

#define MAX_CPU_COUNT 8
uint32_t            cpus_apic_count = 0;
apic_local_cpu_t*   cpus_apic[MAX_CPU_COUNT];

acpi_fadt_t* acpi_get_fadt(){
    return acpi_fadt;
}

uint32_t acpi_get_cpus_apic_count(){
    return cpus_apic_count;
}

apic_local_cpu_t** acpi_get_cpus_apic(){
    return cpus_apic;
}

void acpi_parse_apic_madt(acpi_madt_t* madt){
    acpi_apic = madt;

    uint8_t *p = (uint8_t *)(madt + 1);
    uint8_t *end = (uint8_t *)madt + madt->header.length;

    while(p < end){
        apic_header_t* apic_header = (apic_header_t*)p;
        uint8_t type = apic_header->type;
        
        switch (type)
        {
        case 0:
            //Это контроллер для одного из ядер процессора
            apic_local_cpu_t* cpu_apic = (apic_local_cpu_t*)p;
            cpus_apic[cpus_apic_count++] = cpu_apic;
            break;
        
        default:
            break;
        }

        p += apic_header->length;   
    }
}

void acpi_parse_dt(acpi_header_t* dt)
{
    map_page_mem(V2P(get_kernel_pml4()), dt, dt, PAGE_PRESENT);
    uint32_t signature = dt->signature;

    if (signature == 0x50434146)
    {
        acpi_fadt = dt;
    }
    else if (signature == 0x43495041)
    {
        acpi_parse_apic_madt((acpi_madt_t*)dt);
    }
}

void acpi_read_xsdt(acpi_header_t* xsdt){
    map_page_mem(V2P(get_kernel_pml4()), xsdt, xsdt, PAGE_PRESENT);

    uint64_t *p = (uint64_t *)(xsdt + 1);
    uint64_t *end = (uint64_t *)((uint8_t*)xsdt + xsdt->length);

    while (p < end)
    {
        uint64_t address = *p++;
        acpi_parse_dt(to_acpi_header(address));
    }
}

void acpi_parse_rsdt(acpi_header_t* rsdt){
    map_page_mem(V2P(get_kernel_pml4()), rsdt, rsdt, PAGE_PRESENT);

    uint32_t *p = (uint32_t *)(rsdt + 1);
    uint32_t *end = (uint32_t *)((uint8_t*)rsdt + rsdt->length);

    while (p < end)
    {
        uint32_t address = *(p++);
        acpi_parse_dt(to_acpi_header(address));
    }
}

int acpi_read_rsdp(uint8_t *p)
{
    // Считать контрольную сумму
    uint8_t checksum = 0;
    for (int i = 0; i < 20; ++i)
    {
        checksum += p[i];
    }

    if (checksum)
    {
        printf("ACPI RSDP Checksum failed\n");
        return 0;
    }

    memcpy(acpi_rsdp.oem_id, p + 9, 6);
    acpi_rsdp.oem_id[6] = '\0';

    // Считать версию ACPI
    acpi_rsdp.revision = p[15];
    if (acpi_rsdp.revision == 0)
    {
        acpi_rsdp.rsdt_addr = *(uint32_t*)(&p[16]);
        acpi_parse_rsdt(to_acpi_header(acpi_rsdp.rsdt_addr));
    }
    else if (acpi_rsdp.revision == 2)
    {
        acpi_rsdp.rsdt_addr = *(uint32_t*)(p + 16);
        acpi_rsdp.xsdt_addr = *(uint64_t*)(p + 24);

        if (acpi_rsdp.xsdt_addr)
        {
            acpi_read_xsdt(to_acpi_header(acpi_rsdp.xsdt_addr));
        }
        else
        {
            acpi_parse_rsdt(to_acpi_header(acpi_rsdp.rsdt_addr));
        }
    }
    
    return 1;
}

int acpi_init(){
    memset(&acpi_rsdp, 0, sizeof(acpi_rsdp_t));

    uint8_t *p = (uint8_t *)0x000e0000;
    uint8_t *end = (uint8_t *)0x000fffff;

    while (p < end)
    {
        uint64_t signature = *(uint64_t*)p;

        if (signature == 0x2052545020445352) // 'RSD PTR '
        {
            if (acpi_read_rsdp(p))
            {
                return 0;
            }
        }

        p += 16;
    } 
    return 1;
}

char* acpi_get_oem_str(){
    return acpi_rsdp.oem_id;
}

int acpi_get_revision(){
    if(acpi_rsdp.revision == 0)
        return 1;
    else if(acpi_rsdp.revision == 2)
        return 2;
}