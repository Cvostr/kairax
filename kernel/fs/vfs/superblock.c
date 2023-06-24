#include "superblock.h"
#include "mem/kheap.h"

superblock_t* new_superblock()
{
    superblock_t* result = (superblock_t*)kmalloc(sizeof(superblock_t));
    memset(result, 0, sizeof(superblock_t));
    return result;
}