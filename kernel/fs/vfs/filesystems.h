#ifndef _FILESYSTEMS_H
#define _FILESYSTEMS_H

#include "types.h"
#include "drivers/storage/partitions/storage_partitions.h"
#include "file.h"

struct superblock;

typedef struct {
    const char*         name;
    int                 flags;

    struct inode*        (*mount)(drive_partition_t*, struct superblock*);   //Вызывается при монтировании
    int                  (*unmount)(struct superblock*); //Вызывается при размонтировании
} filesystem_t;

filesystem_t* new_filesystem();

void filesystem_register(filesystem_t* filesystem);

void filesystem_unregister(filesystem_t* filesystem);

filesystem_t* filesystem_get_by_name(const char* name);

#endif