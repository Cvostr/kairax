#ifndef _ACPI_H
#define _ACPI_H

#include "acpi_tables.h"

#define ACPI_ERROR_NO_FADT 1
#define ACPI_ERROR_ENABLE 2

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

acpi_fadt_t* acpi_get_fadt();

uint32_t acpi_fadt_get_smi_cmd_port();

uint32_t acpi_get_cpus_apic_count();

acpi_madt_t* acpi_get_madt();

apic_local_cpu_t** acpi_get_cpus_apic();

apic_io_t*  acpi_get_global_apic();

ioapic_iso_t* acpi_madt_get_iso(uint32_t index);

void acpi_parse_apic_madt(acpi_madt_t* madt);

void acpi_parse_dsdt(acpi_header_t* dsdt);

int acpi_init(void* rsdp_ptr);

char* acpi_get_oem_str();

int acpi_get_revision();

uint16_t acpi_is_enabled();

int acpi_enable();

int acpi_aml_is_struct_valid(uint8_t* struct_address);

void acpi_poweroff();

void acpi_reboot();

void acpi_delay(size_t us);

#endif