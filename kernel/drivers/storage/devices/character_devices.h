#ifndef _CHARACTER_DEVICES_H
#define _CHARACTER_DEVICES_H

#include "types.h"
#include "fs/vfs/file.h"
#include "fs/vfs/inode.h"

#define CHAR_DEVICE_NAME_MAX_LEN     32


typedef struct PACKED {
    char                name[CHAR_DEVICE_NAME_MAX_LEN];
    //file_operations*    operations_ptr;
} character_device_t;

#endif