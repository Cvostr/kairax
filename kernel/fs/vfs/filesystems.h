#ifndef _FILESYSTEMS_H
#define _FILESYSTEMS_H

#include "types.h"
#include "drivers/storage/partitions/storage_partitions.h"
#include "file.h"

typedef struct PACKED {
    const char*         name;
    int                 flags;

    struct inode*        (*mount)(drive_partition_t*);   //Вызывается при монтировании
    int                 (*unmount)(drive_partition_t*); //Вызывается при размонтировании
} filesystem_t;

filesystem_t* new_filesystem();

void filesystem_register(filesystem_t* filesystem);

void filesystem_unregister(filesystem_t* filesystem);

filesystem_t* filesystem_get_by_name(char* name);

#endif