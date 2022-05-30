#ifndef _DEVICES_H
#define _DEVICES_H

#include "stdint.h"

typedef uint64_t    drive_ident_t;

#define DRIVE_DEVICE_CTRL_AHCI 1
#define DRIVE_DEVICE_CTRL_NVME 2
#define DRIVE_DEVICE_CTRL_USB  3

typedef uint64_t    drive_ctrl_t;

typedef struct PACKED {
    char            model[41];      //Имя устройства
    drive_ident_t   ident;          //Идентификатор диска в драйвере контроллера
    drive_ctrl_t    controller;     //Тип контроллера, на котором работает устройство
    uint8_t         uses_lba48;     //Используется ли LBA
    uint32_t        sectors;        //Количество секторов
    uint64_t        bytes;          //Вместимость в байтах  

    uint32_t (*read)(drive_ident_t, uint32_t, uint32_t, char*);
    uint32_t (*write)(drive_ident_t, uint32_t, uint32_t, char*);    

} drive_device_header_t;

void add_storage_device(drive_device_header_t* device);

drive_device_header_t* new_drive_device_header();

uint32_t get_drive_devices_count();

drive_device_header_t** get_drive_devices();

#endif