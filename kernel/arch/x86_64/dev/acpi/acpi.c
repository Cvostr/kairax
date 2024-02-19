#include "acpi.h"
#include "stdio.h"
#include "string.h"
#include "memory/kernel_vmm.h"
#include "mem/pmm.h"
#include "io.h"
#include "dev/hpet/hpet.h"

#define to_acpi_header(x)  (acpi_header_t*)(uintptr_t)(x)

acpi_rsdp_t acpi_rsdp;

acpi_fadt_t* acpi_fadt = NULL;

int is_table_checksum_valid(acpi_header_t* acpi_header)
{
    unsigned char sum = 0;
    for (int i = 0; i < acpi_header->length; i++) {
        sum += ((char*) acpi_header)[i];
    }
    return sum == 0;
}

acpi_fadt_t* acpi_get_fadt()
{
    return acpi_fadt;
}

uint32_t acpi_fadt_get_smi_cmd_port()
{
    return acpi_fadt->smi_cmd_port;
}

void acpi_parse_dt(acpi_header_t* dt)
{
    dt = (acpi_header_t*)P2V(dt);
    uint32_t signature = dt->signature;

    if (signature == 0x50434146)    // FADT in LE
    {
        acpi_fadt = (acpi_fadt_t*)dt;
        if (acpi_get_revision() == 1)
            acpi_parse_dsdt((acpi_header_t*)((uintptr_t)acpi_fadt->dsdt));
        else if (acpi_get_revision() == 2)
            acpi_parse_dsdt((acpi_header_t*)((uintptr_t)acpi_fadt->x_dsdt));
    }
    else if (signature == 0x43495041)   // APIC in LE
    {
        acpi_parse_apic_madt((acpi_madt_t*)dt);
    } 
    else if (signature == 0x54455048) // HPET in LE
    {
        init_hpet((acpi_hpet_t*)dt);
    }
}

void acpi_read_xsdt(acpi_header_t* xsdt)
{
    xsdt = (acpi_header_t*)P2V(xsdt);
    uint64_t *p = (uint64_t *)(xsdt + 1);
    uint64_t *end = (uint64_t *)((uint8_t*)xsdt + xsdt->length);

    while (p < end)
    {
        uint64_t address = *p++;
        acpi_parse_dt(to_acpi_header(address));
    }
}

void acpi_parse_rsdt(acpi_header_t* rsdt)
{
    rsdt = (acpi_header_t*)P2V(rsdt);

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
        return 2;
    }
    
    memcpy(acpi_rsdp.oem_id, p + 9, 6);
    acpi_rsdp.oem_id[6] = '\0';
    
    // Считать версию ACPI
    acpi_rsdp.revision = p[15];
    if (acpi_rsdp.revision == 0)
    {
        acpi_rsdp.rsdt_addr = *(uint32_t*)(&p[16]);
        acpi_parse_rsdt(to_acpi_header(acpi_rsdp.rsdt_addr));
        return 0;
    }
    else if (acpi_rsdp.revision == 2)
    {
        acpi_rsdp.rsdt_addr = *(uint32_t*)(p + 16);
        acpi_rsdp.xsdt_addr = *(uint64_t*)(p + 24);

        if (acpi_rsdp.xsdt_addr)
        {
            acpi_read_xsdt(to_acpi_header(acpi_rsdp.xsdt_addr));
            return 0;
        }
        else
        {
            acpi_parse_rsdt(to_acpi_header(acpi_rsdp.rsdt_addr));
            return 0;
        }
    }
    
    return 3;
}

uint16_t acpi_is_enabled()
{
    return inw((uint32_t) acpi_fadt->pm1a_control_block) & 1;
}

int acpi_enable()
{
    if (!acpi_fadt) {
        return ACPI_ERROR_NO_FADT;
    }

    int acpi_enabled = acpi_is_enabled();
    if (acpi_enabled == 0) {
        //ACPI не включен - включаем
        if (acpi_fadt->smi_cmd_port != 0 && acpi_fadt->acpi_enable) {

            outb((uint32_t)acpi_fadt->smi_cmd_port, acpi_fadt->acpi_enable);

            while(1) {
                io_delay(100);
                acpi_enabled = acpi_is_enabled();
                if (acpi_enabled) {
                    break;
                }
            }
        } else {
            //ACPI не может быть включен
            return ACPI_ERROR_ENABLE;
        }
    }

    return 0;
}

int acpi_init(void* rsdp_ptr)
{
    memset(&acpi_rsdp, 0, sizeof(acpi_rsdp_t));

    if (rsdp_ptr != NULL) {
        return acpi_read_rsdp(rsdp_ptr);
    }

    uint8_t *p = (uint8_t *)P2V(0x000e0000);
    uint8_t *end = (uint8_t *)P2V(0x000fffff);

    while (p < end)
    {
        uint64_t signature = *(uint64_t*)p;
        if (signature == 0x2052545020445352) // 'RSD PTR '
        {
            return acpi_read_rsdp(p);
        }

        p += 16;
    }

    return 1;
}

char* acpi_get_oem_str()
{
    return acpi_rsdp.oem_id;
}

int acpi_get_revision()
{
    if(acpi_rsdp.revision == 0)
        return 1;
    else if(acpi_rsdp.revision == 2)
        return 2;
}

#define PMT_TIMER_RATE 3579545 // 3.57 MHz

int acpi_delay(size_t us)
{
    if (acpi_fadt->pm_timer_length != 4){
        printk("Error: ACPI Timer not available\n");
        return -1;
    }
    
    size_t count = inl(acpi_fadt->pm_timer_block);
    size_t end = (us * PMT_TIMER_RATE) / 100;
    size_t current = 0;

    while (current < end) {
        current = ((inl(acpi_fadt->pm_timer_block) - count) & 0xffffff);
    }

    return 0;
}