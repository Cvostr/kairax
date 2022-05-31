#ifndef _STORAGE_PARTITIONS_H
#define _STORAGE_PARTITIONS_H

#include "../devices/storage_devices.h"

typedef struct PACKED {
    char                    name[15];       //Имя устройства в системе  
    drive_device_header_t*  device;
    uint32_t                index;

    uint32_t                start_sector;
    uint32_t                sectors;

} drive_partition_header_t;

drive_partition_header_t* new_drive_partition_header();

void add_partitions_from_device(drive_device_header_t* device);

void add_partition_header(drive_partition_header_t* partition_header);

uint32_t get_partitions_count();

drive_partition_header_t* get_partition(uint32_t index);

#endif