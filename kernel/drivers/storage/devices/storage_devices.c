#include "storage_devices.h"
#include "mem/kheap.h"
#include "raxlib/list/list.h"
#include "string.h"

#define MAX_STOR_DEVICES_HEADERS 300

list_t* drives = NULL;

void add_storage_device(drive_device_t* device){
    if(drives == NULL)
        drives = create_list();

    list_add(drives, device);
}

drive_device_t* new_drive_device_header(){
    drive_device_t* result = (drive_device_t*)kmalloc(sizeof(drive_device_t));
    memset(result, 0, sizeof(drive_device_t));

    return result;
}

uint32_t get_drive_devices_count(){
    if(drives == NULL)
        return 0;
    return drives->size;
}

drive_device_t* get_drive(uint32_t index){
    return (drive_device_t*)list_get(drives, index);
}

uint32_t drive_device_read( drive_device_t* drive,
                            uint64_t start_lba,
                            uint64_t count,
                            char* buffer)
{
    return drive->read(drive->ident, start_lba, count, buffer);
}

uint32_t drive_device_write(drive_device_t* drive,
                            uint64_t start_lba,
                            uint64_t count,
                            char* buffer)
{
    return drive->write(drive->ident, start_lba, count, buffer);
}

