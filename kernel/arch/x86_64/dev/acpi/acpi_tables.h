#ifndef _ACPI_TABLES_H
#define _ACPI_TABLES_H

#include "stdint.h"

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

//APIC

typedef struct PACkED
{
    acpi_header_t header;
    uint32_t local_apic_address;
    uint32_t flags;
} acpi_madt_t;

typedef struct PACKED
{
    uint8_t type;
    uint8_t length;
} apic_header_t;

typedef struct PACKED
{
    acpi_header_t header;
    uint8_t acpi_cpu_id;
    uint8_t apicId;
    uint32_t flags;
} apic_local_cpu_t;

//FADT

typedef struct PACKED
{
    acpi_header_t   header;
    uint32_t        firmware_ctrl;
    uint32_t        dsdt;
    uint8_t         reserved;
    uint8_t         preferred_pwr_mgmt_profile;
    uint16_t        sci_interrupt;
    uint32_t        smi_cmd_port;
    uint8_t         acpi_enable;
    uint8_t         acpi_disable;
    uint8_t         s4bios_req;
    uint8_t         pstate_ctrl;

    uint32_t        pm1a_event_block;
    uint32_t        pm1b_event_block;
    uint32_t        pm1a_control_block;
    uint32_t        pm1b_control_block;
    uint32_t        pm2_control_block;
    uint32_t        pm_timer_block;

    uint32_t        gpe0_block;
    uint32_t        gpe1_lock;
    uint8_t         pm1_event_length;
    uint8_t         pm1_control_length;
    uint8_t         pm2_control_length;
    uint8_t         pm_timer_length;
    uint8_t         gpe0_length;
    uint8_t         gpe1_ength;
    uint8_t         gpe1_base;
    uint8_t         cstate_ctrl;
    uint16_t        worst_c2_atency;
    uint16_t        worst_c3_atency;
    uint16_t        flush_size;
    uint16_t        flush_stride;
    uint8_t         duty_offset;
    uint8_t         duty_width;
    uint8_t         day_alarm;
    uint8_t         month_alarm;
    uint8_t         century;

    uint16_t        boot_architecture_flags;
} acpi_fadt_t;

#endif