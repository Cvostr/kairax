#include "storage_devices.h"
#include "mem/kheap.h"
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