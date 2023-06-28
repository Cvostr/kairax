#include "superblock.h"
#include "mem/kheap.h"

struct superblock* new_superblock()
{
    struct superblock* result = (struct superblock*)kmalloc(sizeof(struct superblock));
    memset(result, 0, sizeof(struct superblock));
    result->inodes = create_list();
    return result;
}

void free_superblock(struct superblock* sb)
{
    free_list(sb->inodes);
    kfree(sb);
}