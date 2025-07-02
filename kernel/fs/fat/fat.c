#include "../vfs/filesystems.h"
#include "fat.h"
#include "stdio.h"
#include "mem/kheap.h"

struct super_operations fat_sb_ops;

struct file_operations fat_dir_ops;

void fat_init()
{
    filesystem_t* fatfs = new_filesystem();
    fatfs->name = "fat";
    fatfs->mount = fat_mount;
    fatfs->unmount = fat_unmount;

    filesystem_register(fatfs);

    fat_sb_ops.stat = fat_statfs;

    fat_dir_ops.readdir = fat_file_readdir;
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

    struct fat_bpb* bpb = kmalloc(bsize); 
    instance->bpb = bpb;
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
        printk("FATS is 0!\n");
        fat_free_instance(instance);
        return NULL;
    }

    // Сохранить кол-во секторов
    instance->sectors_count = (bpb->total_sectors16 == 0) ? bpb->total_sectors32 : bpb->total_sectors16;

    // Сохранить размер FAT (в секторах)
    instance->fat_size = (bpb->fat_size16 == 0) ? bpb->ext_32.fat_size : bpb->fat_size16;

    // Размер корневой директории
    instance->root_dir_sectors = ((bpb->root_entry_count * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;

    instance->first_fat_sector = bpb->reserved_sector_count;
    instance->first_data_sector = bpb->reserved_sector_count + (bpb->fats_count * instance->fat_size) + instance->root_dir_sectors;

    // Количество секторов, отведенных под данные (вычитаем все FATs, зарезервированные и корневую директорию)
    instance->data_sectors = instance->sectors_count - (bpb->reserved_sector_count + (bpb->fats_count * instance->fat_size) + instance->root_dir_sectors);

    // Количество кластеров
    instance->total_clusters = instance->data_sectors / bpb->sectors_per_cluster;

    // Определить тип файловой системы
    if (bpb->bytes_per_sector == 0) {
        printk("exFAT is unsupported!\n");
        fat_free_instance(instance);
        return NULL;
    } else if(instance->total_clusters < 4085) {
        instance->fs_type = FS_FAT12;
    } else if(instance->total_clusters < 65525) {
        instance->fs_type = FS_FAT16;
    } else {
        instance->fs_type = FS_FAT32;
    }

    printk("FAT: type %i FATs %i FAT size %i sectors %i BPS %i\n", instance->fs_type, bpb->fats_count, instance->fat_size, instance->sectors_count, bpb->bytes_per_sector);

    struct inode* result = new_vfs_inode();
    result->inode = 2;
    result->sb = sb;
    result->uid = 0;
    result->gid = 0;
    result->size = 0;
    result->mode = INODE_TYPE_DIRECTORY;
    result->access_time = 0;
    result->create_time = 0;
    result->modify_time = 0;
    result->hard_links = 1;
    result->device = (dev_t) instance;
    result->file_ops = &fat_dir_ops;

    sb->fs_info = instance;
    //sb->blocksize = instance->block_size;
    sb->operations = &fat_sb_ops;

    return result;
}

struct dirent* fat_file_readdir(struct file* dir, uint32_t index)
{
    printk("fat_file_readdir");
}

void fat_read_direntry(struct fat_instance* instance, struct fat_direntry* direntry, uint32_t cluster_idx, uint32_t entry_idx)
{
    uint32_t bufsz = instance->fs_type == FS_FAT12 ? instance->bpb->bytes_per_sector * 2 : instance->bpb->bytes_per_sector; 
    char* fat = kmalloc(bufsz);
}

int fat_statfs(struct superblock *sb, struct statfs* stat)
{
    struct fat_instance* inst = (struct fat_instance*) sb->fs_info;

    stat->blocksize = inst->bpb->bytes_per_sector;

    stat->blocks = inst->sectors_count;
    stat->blocks_free = 0;

    return 0;
}

int fat_unmount(struct superblock* sb)
{

}