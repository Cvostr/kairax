#ifndef _ACPI_H
#define _ACPI_H

#include "acpi_tables.h"

typedef struct PACKED {
    char        signature[8];
    uint8_t     checksum;
    char        oem_id[7];
    uint8_t     revision;
    uint32_t    rsdt_addr;
    //Следующие поля для ACPI версии 2.0
    uint32_t    length;
    uint64_t    xsdt_addr;
    uint8_t     ext_checksum;
    uint8_t     reserved[3];
} acpi_rsdp_t;

void acpi_parse_apic_madt(acpi_madt_t* madt);

int acpi_init();

#endif