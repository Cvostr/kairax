#include "acpi.h"
#include "memory/paging.h"
#include "io.h"

acpi_header_t* acpi_dsdt;

uint16_t SLP_TYPa;
uint16_t SLP_TYPb;

int acpi_aml_is_struct_valid(uint8_t* struct_address){
    return  (*(struct_address - 1) == 0x08 || (*(struct_address - 2) == 0x08 && *(struct_address - 1) == '\\')) &&
            *(struct_address + 4) == 0x12;
}

void acpi_parse_dsdt(acpi_header_t* dsdt) {
    acpi_dsdt = dsdt;
    char* data = (char*)(dsdt);
    map_page_mem(V2P(get_kernel_pml4()), dsdt, dsdt, PAGE_PRESENT);

    char* data_address = (char*)(dsdt + 1);
    uint32_t dsdt_size = dsdt->length;

    for(uint32_t i = 1; i < dsdt_size / 4096; i ++){
        map_page_mem(V2P(get_kernel_pml4()), data + i * 4096, data + i * 4096, PAGE_PRESENT);
    }
    
    int it = 0;
    
    while(it < dsdt_size){
        data_address++;
        if(memcmp(data_address, "_S5_", 4) == 0){
            if(acpi_aml_is_struct_valid(data_address)){
                //printf("POWEROFF TABLE FOUND \n");
                data_address += 5;
                data_address += ((*data_address & 0xC0) >> 6) + 2;

                if (*data_address == 0x0A)
                    data_address++;

                SLP_TYPa = *(data_address) << 10;
                data_address++;

                if (*data_address == 0x0A)
                    data_address++;  
                
                SLP_TYPb = *(data_address)<<10;
            }
        }else if(memcmp(data_address, "_S1_", 4) == 0){
            if(acpi_aml_is_struct_valid(data_address)){
                printf("SLEEP TABLE FOUND \n");
            }
        }
        it++;
    }
}

void acpi_poweroff(){
    uint32_t SLP_EN = 1 << 13;
    outw((uint32_t) acpi_get_fadt()->pm1a_control_block, SLP_TYPa | SLP_EN );
    //Если выключить компьютер через блок А не получилось, пробуем блок B
    outw((uint32_t) acpi_get_fadt()->pm1b_control_block, SLP_TYPb | SLP_EN );
    //Мы что, до сих пор работаем?? вот незадача
}