#ifndef _DEVICES_H
#define _DEVICES_H

#include "types.h"

typedef uint64_t    drive_ident_t;

#define DRIVE_DEVICE_CTRL_AHCI 1
#define DRIVE_DEVICE_CTRL_NVME 2
#define DRIVE_DEVICE_CTRL_USB  3

typedef uint64_t    drive_ctrl_t;

typedef struct {
    char            name[12];       //Имя устройства в системе
    char            model[41];      //Модель устройства
    char            serial[35];     //Серийный номер устройства
    drive_ident_t   ident;          //Идентификатор диска в драйвере контроллера
    drive_ctrl_t    controller;     //Тип контроллера, на котором работает устройство
    uint8_t         uses_lba48;     //Используется ли LBA
    uint32_t        sectors;        //Количество секторов
    uint64_t        bytes;          //Вместимость в байтах  

    uint32_t (*read)(drive_ident_t, uint64_t, uint64_t, char*);
    uint32_t (*write)(drive_ident_t, uint32_t, uint64_t, char*);    

} drive_device_t;

void add_storage_device(drive_device_t* device);

drive_device_t* new_drive_device_header();

uint32_t get_drive_devices_count();

drive_device_t* get_drive(uint32_t index);

uint32_t drive_device_read( drive_device_t* drive,
                            uint64_t start_lba,
                            uint64_t count,
                            char* buffer);

uint32_t drive_device_write( drive_device_t* drive,
                            uint64_t start_lba,
                            uint64_t count,
                            char* buffer);
#endif