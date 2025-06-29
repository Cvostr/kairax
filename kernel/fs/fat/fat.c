#include "../vfs/filesystems.h"
#include "fat.h"
#include "stdio.h"
#include "mem/kheap.h"

void fat_init()
{
    filesystem_t* fatfs = new_filesystem();
    fatfs->name = "fat";
    fatfs->mount = fat_mount;
    fatfs->unmount = fat_unmount;

    filesystem_register(fatfs);
}

uint32_t fat_read_block(struct fat_instance* inst, uint64_t block_start, uint64_t blocks, char* buffer)
{
    //uint64_t    start_lba = block_start * (inst->block_size / 512);
    //uint64_t    lba_count = blocks  * (inst->block_size / 512);
    //return partition_read(inst->partition, start_lba, lba_count, buffer);
}

void fat_free_instance(struct fat_instance* instance)
{
    KFREE_SAFE(instance->bpb)
    kfree(instance);
}

struct inode* fat_mount(drive_partition_t* drive, struct superblock* sb)
{
    struct fat_instance* instance = kmalloc(sizeof(struct fat_instance));
    memset(instance, 0, sizeof(struct fat_instance));

    instance->vfs_sb = sb;
    instance->partition = drive;

    uint64_t bsize = drive->device->drive_info->block_size;
    //printk("FAT bsize %i\n", bsize);
    if (bsize < 512)
    {
        fat_free_instance(instance);
        return NULL;
    }

    instance->bpb = kmalloc(bsize);
    memset(instance->bpb, 0, bsize);

    partition_read(drive, 0, 1, instance->bpb);

    uint8_t* jmp_op = instance->bpb->jump_op;
    int jmp_op_valid = (jmp_op[0] == 0xEB && jmp_op[2] == 0x90) || (jmp_op[0] == 0xE9);
    if (jmp_op_valid == FALSE)
    {
        fat_free_instance(instance);
        return NULL;
    }
   
    if (instance->bpb->fats_count == 0)
    {
        fat_free_instance(instance);
        return NULL;
    }

    return NULL;
}

int fat_unmount(struct superblock* sb)
{

}