#include "acpi.h"
#include "memory/paging.h"
#include "io.h"
#include "string.h"
#include "aml.h"

acpi_header_t* acpi_dsdt;

uint16_t SLP_TYPa;
uint16_t SLP_TYPb;

int acpi_aml_is_struct_valid(uint8_t* struct_address)
{
    return  (*(struct_address - 1) == 0x08 || (*(struct_address - 2) == 0x08 && *(struct_address - 1) == '\\')) &&
            *(struct_address + 4) == 0x12;
}

void acpi_parse_dsdt(acpi_header_t* dsdt) 
{
    dsdt = P2V(dsdt);
    acpi_dsdt = dsdt;
    char* data = (char*)(dsdt);

    char* data_address = (char*)(dsdt + 1);
    uint32_t dsdt_size = dsdt->length;

    int parse_rc = aml_parse(data_address, dsdt_size - sizeof(acpi_header_t));
    if (parse_rc != 0)
        printk("ACPI: AML parsing error %i", parse_rc);
    
    int it = 0;
    
    while(it < dsdt_size){
        data_address++;
        if(memcmp(data_address, "_S5_", 4) == 0) {
            if(acpi_aml_is_struct_valid(data_address)){

                data_address += 5;
                data_address += ((*data_address & 0xC0) >> 6) + 2;

                if (*data_address == 0x0A)
                    data_address++;

                SLP_TYPa = *(data_address) << 10;
                data_address++;

                if (*data_address == 0x0A)
                    data_address++;  
                
                SLP_TYPb = *(data_address) << 10;
            }
        } else if(memcmp(data_address, "_S1_", 4) == 0){
            if(acpi_aml_is_struct_valid(data_address)){
                // Сон
            }
        }
        it++;
    }
}

void acpi_poweroff()
{
    uint32_t SLP_EN = 1 << 13;
    outw((uint32_t) acpi_get_fadt()->pm1a_control_block, SLP_TYPa | SLP_EN );
    //Если выключить компьютер через блок А не получилось, пробуем блок B
    outw((uint32_t) acpi_get_fadt()->pm1b_control_block, SLP_TYPb | SLP_EN );
    //Мы что, до сих пор работаем?? вот незадача
}

void acpi_reboot()
{
    if (acpi_get_revision() == 2) {
        // Способ ACPI 2.0
        uint8_t* reset_addr = (uint8_t*) P2V(acpi_get_fadt()->reset_reg.address);
        *reset_addr = acpi_get_fadt()->reset_value;

        // Для не memory mapped
        outb(acpi_get_fadt()->reset_reg.address, acpi_get_fadt()->reset_value);
    }
}