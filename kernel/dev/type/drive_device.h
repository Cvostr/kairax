#ifndef _DISK_DEVICE_H
#define _DISK_DEVICE_H

#include "kairax/types.h"

struct drive_device_info {

    char            blockdev_name[12];  //Имя устройства в системе
    char            serial[35];         //Серийный номер устройства
    uint8_t         uses_lba48;         //Используется ли LBA
    uint32_t        sectors;            //Количество секторов
    uint64_t        nbytes;             //Вместимость в байтах  

    int (*read) (struct device*, uint64_t, uint64_t, char*);
    int (*write) (struct device*, uint64_t, uint64_t, const char*);    
};

#endif