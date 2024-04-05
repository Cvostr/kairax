#include "mem/kheap.h"
#include "list/list.h"
#include "string.h"
#include "dev/device.h"

int drive_device_read( struct device* drive,
                            uint64_t start_lba,
                            uint64_t count,
                            char* buffer)
{
    return drive->drive_info->read(drive, start_lba, count, buffer);
}

int drive_device_write(struct device* drive,
                            uint64_t start_lba,
                            uint64_t count,
                            char* buffer)
{
    return drive->drive_info->write(drive, start_lba, count, buffer);
}

