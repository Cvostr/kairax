#ifndef _FILESYSTEMS_H
#define _FILESYSTEMS_H

#include "stdint.h"
#include "drivers/storage/partitions/storage_partitions.h"

typedef struct PACKED {
    const char*     name;
    int             flags;

    int             (*mount)(drive_partition_t*);
    int             (*unmount)(drive_partition_t*);
} filesystem_t;

void filesystem_register(filesystem_t* filesystem);

void filesystem_unregister(filesystem_t* filesystem);

#endif