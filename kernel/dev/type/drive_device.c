#include "drive_device.h"
#include "mem/kheap.h"
#include "string.h"

struct drive_device_info* new_drive_device_info()
{
    struct drive_device_info* result = kmalloc(sizeof(struct drive_device_info));
    memset(result, 0, sizeof(struct drive_device_info));
    return result;
}