#ifndef _DISK_DEVICE_H
#define _DISK_DEVICE_H

#include "kairax/types.h"

struct device;

struct drive_device {
    char            blockdev_name[12];  //Имя устройства в системе
    char            serial[35];         //Серийный номер устройства
    uint8_t         uses_lba48;         //Используется ли LBA
    uint32_t        sectors;            //Количество секторов
    uint64_t        bytes;              //Вместимость в байтах  

    uint32_t (*read) (struct device*, uint64_t, uint64_t, char*);
    uint32_t (*write) (struct device*, uint32_t, uint64_t, char*);    
};

#endif