#ifndef _DISK_DEVICE_H
#define _DISK_DEVICE_H

#include "kairax/types.h"

#define BLOCKDEV_NAMELEN    20

#define DRIVE_REMOVABLE  0x4

struct device;

struct drive_device_info {

    char            blockdev_name[BLOCKDEV_NAMELEN + 1];  //Имя устройства в системе
    char            serial[35];         //Серийный номер устройства
    int             flags;
    uint8_t         uses_lba48;         //Используется ли LBA
    uint64_t        sectors;            //Количество секторов
    uint64_t        nbytes;             //Вместимость в байтах  
    uint64_t        block_size;         // Размер 1 блока в байтах

    struct device*  dev;

    int (*read) (struct device*, uint64_t, uint64_t, char*);
    int (*write) (struct device*, uint64_t, uint64_t, const char*);    
};

struct drive_device_info* new_drive_device_info();

int register_drive(struct drive_device_info* drive);

int drive_device_read(  struct drive_device_info* drive,
                        uint64_t start_lba,
                        uint64_t count,
                        char* buffer);

int drive_device_write( struct drive_device_info* drive,
                        uint64_t start_lba,
                        uint64_t count,
                        const char* buffer);

#endif