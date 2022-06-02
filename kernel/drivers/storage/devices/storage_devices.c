#include "storage_devices.h"
#include "mem/kheap.h"
#include "raxlib/list/list.h"
#include "string.h"

#define MAX_STOR_DEVICES_HEADERS 300

drive_device_header_t* devices_headers[MAX_STOR_DEVICES_HEADERS];
uint32_t storage_devices_count = 0;

void add_storage_device(drive_device_header_t* device){
    devices_headers[storage_devices_count++] = device;
}

drive_device_header_t* new_drive_device_header(){
    drive_device_header_t* result = (drive_device_header_t*)kmalloc(sizeof(drive_device_header_t));
    memset(result, 0, sizeof(drive_device_header_t));

    return result;
}

uint32_t get_drive_devices_count(){
    return storage_devices_count;
}

drive_device_header_t** get_drive_devices(){
    return devices_headers;
}

uint32_t drive_device_read( drive_device_header_t* drive,
                            uint32_t start_sector_low,
                            uint32_t start_sector_high,
                            uint32_t end_sector,
                            char* buffer)
{
    return drive->read(drive->ident, start_sector_low, start_sector_high, end_sector, buffer);
}

uint32_t drive_device_read1( drive_device_header_t* drive,
                            uint64_t start_lba,
                            uint32_t end_sector,
                            char* buffer)
{
    return drive->read(drive->ident, (uint32_t)start_lba, (uint32_t)(start_lba >> 32), end_sector, buffer);
}

uint32_t drive_device_write(drive_device_header_t* drive,
                            uint32_t start_sector_low,
                            uint32_t start_sector_high,
                            uint32_t end_sector,
                            char* buffer)
{
    return drive->write(drive->ident, start_sector_low, start_sector_high, end_sector, buffer);
}