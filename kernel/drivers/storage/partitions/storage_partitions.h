#ifndef _STORAGE_PARTITIONS_H
#define _STORAGE_PARTITIONS_H

#include "../devices/storage_devices.h"

typedef struct PACKED {
    char                    name[15];       //Имя устройства в системе  
    drive_device_t*         device;
    uint32_t                index;

    uint64_t                start_lba;
    uint64_t                sectors;

} drive_partition_t;

drive_partition_t* new_drive_partition_header();

void add_partitions_from_device(drive_device_t* device);

void add_partition_header(drive_partition_t* partition_header);
//Количество разделов, подключенных в данный момент
uint32_t get_partitions_count();
//Получить раздел по номеру
drive_partition_t* get_partition(uint32_t index);
//Получить раздел по указанному имени
drive_partition_t* get_partition_with_name(const char* name);

uint32_t   partition_read(drive_partition_t*, uint64_t lba_start, uint64_t count, char* buffer);

uint32_t   partition_write(drive_partition_t*, uint64_t lba_start, uint64_t count, char* buffer);

#endif