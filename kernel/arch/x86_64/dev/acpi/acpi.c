#include "acpi.h"
#include "stdio.h"
#include "string.h"

acpi_rsdp_t acpi_rsdp;

void acpi_parse_dt(uint32_t dt_addr)
{
    acpi_header_t* dt = (acpi_header_t*)dt_addr;
    uint32_t signature = dt->signature;
}

void acpi_parse_rsdt(uint32_t rsdt_addr){
    acpi_header_t* rsdt = (acpi_header_t*)rsdt_addr;
    uint32_t *p = (uint32_t *)(rsdt + 1);
    uint32_t *end = (uint32_t *)((uint8_t*)rsdt + rsdt->length);

    /*while (p < end)
    {
        uint32_t address = *p++;
        //acpi_parse_dt(address);
    }*/
}

int acpi_read_rsdp(uint8_t *p)
{
    printf("ACPI RSDP found\n");
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
    printf("ACPI OEM = %s\n", acpi_rsdp.oem_id);

    // Считать версию ACPI
    acpi_rsdp.revision = p[15];
    if (acpi_rsdp.revision == 0)
    {
        printf("ACPI Version 1\n");
        acpi_rsdp.rsdt_addr = *(uint32_t*)(p + 16);
        //acpi_parse_rsdt(acpi_rsdp.rsdt_addr);
    }
    else if (acpi_rsdp.revision == 2)
    {
        printf("ACPI Version 2\n");

        acpi_rsdp.rsdt_addr = *(uint32_t*)(p + 16);
        acpi_rsdp.xsdt_addr = *(uint64_t*)(p + 24);

        if (acpi_rsdp.xsdt_addr)
        {
           //parse xsdt
        }
        else
        {
            acpi_parse_rsdt(acpi_rsdp.rsdt_addr);
        }
    }
    
    return 1;
}

int acpi_init(){
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