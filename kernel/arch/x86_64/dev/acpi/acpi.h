#ifndef _ACPI_H
#define _ACPI_H

#include "stdint.h"

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

typedef struct PACKED
{
    uint32_t    signature;
    uint32_t    length;
    uint8_t     revision;
    uint8_t     checksum;
    uint8_t     oem[6];
    uint8_t     oemTableId[8];
    uint32_t    oemRevision;
    uint32_t    creatorId;
    uint32_t    creatorRevision;
} acpi_header_t;


int acpi_init();

#endif