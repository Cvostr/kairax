#ifndef _DISK_DEVICE_H
#define _DISK_DEVICE_H

#include "kairax/types.h"

#define DRIVE_REMOVABLE  0x4

struct drive_device_info {

    char            blockdev_name[12];  //Имя устройства в системе
    char            serial[35];         //Серийный номер устройства
    int             flags;
    uint8_t         uses_lba48;         //Используется ли LBA
    uint64_t        sectors;            //Количество секторов
    uint64_t        nbytes;             //Вместимость в байтах  
    uint64_t        block_size;         // Размер 1 блока в байтах

    int (*read) (struct device*, uint64_t, uint64_t, char*);
    int (*write) (struct device*, uint64_t, uint64_t, const char*);    
};

struct drive_device_info* new_drive_device_info();

#endif