#include "../vfs/filesystems.h"
#include "fat.h"
#include "stdio.h"
#include "mem/kheap.h"
#include "kairax/stdio.h"
#include "string.h"

struct super_operations fat_sb_ops;

struct file_operations fat_dir_ops;

void fat_init()
{
    filesystem_t* fatfs = new_filesystem();
    fatfs->name = "fat";
    fatfs->mount = fat_mount;
    fatfs->unmount = fat_unmount;

    filesystem_register(fatfs);

    fat_sb_ops.read_inode = fat_read_node;
    fat_sb_ops.stat = fat_statfs;
    fat_sb_ops.find_dentry = fat_find_dentry;

    fat_dir_ops.readdir = fat_file_readdir;
}

uint32_t fat_read_sector(struct fat_instance* inst, uint64_t block_start, uint64_t blocks, char* buffer)
{
    uint32_t    bps = inst->bytes_per_sector;
    uint64_t    start_lba = block_start * (bps / 512);
    uint64_t    lba_count = blocks  * (bps / 512);
    
    return partition_read(inst->partition, start_lba, lba_count, buffer);
}

uint32_t fat_read_cluster(struct fat_instance* inst, uint32_t cluster, char* buffer)
{
    uint32_t    spc = inst->bpb->sectors_per_cluster;
    uint32_t    bps = inst->bytes_per_sector;

    uint64_t    start_lba = cluster * spc * (bps / 512);
    uint64_t    lba_count = spc * (bps / 512);
    
    return partition_read(inst->partition, start_lba, lba_count, buffer);
}

void fat_free_instance(struct fat_instance* instance)
{
    KFREE_SAFE(instance->bpb)
    KFREE_SAFE(instance->fsinfo32)
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

    if (instance->bpb->boot_part_signature != BOOTABLE_PARTITION_SIGNATURE)
    {
        printk("Boot partition signature is incorrect!\n");
        fat_free_instance(instance);
        return NULL;
    }

    instance->bytes_per_sector = bpb->bytes_per_sector;
    // Сохранить кол-во секторов
    instance->sectors_count = (bpb->total_sectors16 == 0) ? bpb->total_sectors32 : bpb->total_sectors16;

    // Сохранить размер FAT (в секторах)
    instance->fat_size = (bpb->fat_size16 == 0) ? bpb->ext_32.fat_size : bpb->fat_size16;

    // Размер корневой директории
    uint32_t root_dir_sector = bpb->reserved_sector_count + (bpb->fats_count * instance->fat_size);
    instance->root_dir_sectors = ((bpb->root_entry_count * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;

    instance->first_fat_sector = bpb->reserved_sector_count;
    instance->first_data_sector = bpb->reserved_sector_count + (bpb->fats_count * instance->fat_size) + instance->root_dir_sectors;

    // Количество секторов, отведенных под данные (вычитаем все FATs, зарезервированные и корневую директорию)
    instance->data_sectors = instance->sectors_count - (bpb->reserved_sector_count + (bpb->fats_count * instance->fat_size) + instance->root_dir_sectors);

    // Количество кластеров
    instance->total_clusters = instance->data_sectors / bpb->sectors_per_cluster;

    // Определить тип файловой системы
    if (bpb->bytes_per_sector == 0) {
        instance->fs_type = FS_EXFAT;
        // пока не реализовано полноценно
        printk("exFAT is unsupported!\n");
        fat_free_instance(instance);
        return NULL;
    } else if(instance->total_clusters < 4085) {
        instance->fs_type = FS_FAT12;
    } else if(instance->total_clusters < 65525) {
        instance->fs_type = FS_FAT16;
    } else {
        instance->fs_type = FS_FAT32;
        // Чтение структуры FSINFO32
        instance->fsinfo32 = kmalloc(bsize);
        partition_read(drive, 0, 1, instance->fsinfo32);
        // Проверка
        if (instance->fsinfo32->lead_signature != FSINFO32_SIGNATURE || instance->fsinfo32->trail_signature != FSINFO32_TRAIL_SIGNATURE)
        {
            printk("Fat32 FSINFO structure signatures incorrect!\n");
            fat_free_instance(instance);
            return NULL;
        }
    }

    printk("FAT: type %i FATs %i FAT size %i FirstFat %i sectors %i BPS %i clusters %i SPC %i\n", 
        instance->fs_type, 
        bpb->fats_count,
        instance->fat_size,
        instance->first_fat_sector,
        instance->sectors_count,
        bpb->bytes_per_sector,
        instance->total_clusters,
        bpb->sectors_per_cluster);

    // Собрать номер иноды из номера сектора и смещения
    uint64_t ino_num = ((uint64_t)root_dir_sector) << 32 | 0;

    struct fat_direntry root_direntry;
    fat_read_dentry(instance, ino_num, &root_direntry, NULL);
    printk("FAT: Root dentry name %s cluster %i size %i\n", root_direntry.name, FAT_DIRENTRY_GET_CLUSTER(root_direntry), root_direntry.file_size);

    struct inode* result = new_vfs_inode();
    result->inode = ino_num;
    result->sb = sb;
    result->uid = 0;
    result->gid = 0;
    result->size = root_direntry.file_size;
    result->mode = INODE_TYPE_DIRECTORY;
    result->access_time = 0;
    result->create_time = 0;
    result->modify_time = 0;
    result->hard_links = 1;
    result->device = (dev_t) instance;
    result->file_ops = &fat_dir_ops;

    sb->fs_info = instance;
    sb->blocksize = bpb->bytes_per_sector;
    sb->operations = &fat_sb_ops;

    return result;
}

int fat_read_dentry(struct fat_instance* inst, uint64_t inode_num, struct fat_direntry* dentry, char* lfn_buffer) 
{
    uint32_t sector = inode_num >> 32;
    uint32_t offset = inode_num & UINT32_MAX;

    char* sector_temp = kmalloc(inst->bpb->bytes_per_sector);

    int rc = fat_read_sector(inst, sector, 1, sector_temp);
    if (rc != 0)
        return rc;

    uint32_t pos = offset;

    struct fat_direntry*    d_ptr;
    struct fat_lfn*         d_lfn;

    while (TRUE) {
        d_ptr = &sector_temp[pos];

        if (d_ptr->attr == FILE_ATTR_LFN) {
            d_lfn = d_ptr;
            printk("LFN");

            pos += sizeof(struct fat_lfn);
            continue;
        }

        memcpy(dentry, d_ptr, sizeof(struct fat_direntry));
        break;
    }

    kfree(sector_temp);
    return 0;
}

uint32_t fat_get_next_cluster(struct fat_instance* inst, uint32_t cluster)
{
    uint32_t result = 0;
    char* fat_buffer = NULL;
    uint32_t fat_offset = 0;    // Смещение внутри таблицы FAT в байтах
    uint32_t fat_sector = 0;    // Номер сектора в таблице FAT (Смещение внутри таблицы FAT в секторах)
    uint32_t sector_offset = 0; // Смещение внутри сектора таблицы FAT

    switch (inst->fs_type)
    {
        case FS_FAT12:
            fat_buffer = kmalloc(inst->bytes_per_sector * 2);
            fat_offset = cluster + (cluster / 2);  
            fat_sector = inst->first_fat_sector + (fat_offset / inst->bytes_per_sector); 
            sector_offset = fat_offset % inst->bytes_per_sector;
            
            fat_read_sector(inst, fat_sector, 2, fat_buffer);
            
            result = *((uint16_t*) &fat_buffer [sector_offset]);
            result = (cluster & 1) ? result >> 4 : result & 0xfff;
            break;
        case FS_FAT16:
            fat_buffer = kmalloc(inst->bytes_per_sector);
            fat_offset = cluster * sizeof(uint16_t);
            fat_sector = inst->first_fat_sector + (fat_offset / inst->bytes_per_sector);
            sector_offset = fat_offset % inst->bytes_per_sector;

            fat_read_sector(inst, fat_sector, 1, fat_buffer);

            result = result = *((uint16_t*) &fat_buffer [sector_offset]);
            break;
        case FS_FAT32:
        case FS_EXFAT:
            fat_buffer = kmalloc(inst->bytes_per_sector);
            fat_offset = cluster * sizeof(uint32_t);
            fat_sector = inst->first_fat_sector + (fat_offset / inst->bytes_per_sector);
            sector_offset = fat_offset % inst->bytes_per_sector;

            fat_read_sector(inst, fat_sector, 1, fat_buffer);

            result = result = *((uint32_t*) &fat_buffer [sector_offset]);
            if (inst->fs_type == FS_FAT32)
            {
                result = result &= 0x0FFFFFFF;
            }
            break;
    }

    KFREE_SAFE(fat_buffer);
    return result;
}

struct inode* fat_read_node(struct superblock* sb, uint64_t ino_num)
{
    printk("fat_read_node\n");
    return NULL;
}

struct dirent* fat_file_readdir(struct file* dir, uint32_t index)
{
    struct inode* vfs_inode = dir->inode;
    struct superblock* sb = vfs_inode->sb;
    struct fat_instance* inst = sb->fs_info;

    // Считать direntry
    struct fat_direntry direntry;
    fat_read_dentry(inst, vfs_inode->inode, &direntry, NULL);

    char* cluster_buffer = kmalloc(inst->bpb->sectors_per_cluster * inst->bytes_per_sector);
    // Текущий кластер
    uint32_t current_cluster = FAT_DIRENTRY_GET_CLUSTER(direntry);
    // Следующий кластер
    uint32_t next_cluster = fat_get_next_cluster(inst, current_cluster);

    fat_read_cluster(inst, current_cluster, cluster_buffer);

    // TODO IMPLEMENT

    printk("fat_file_readdir\n");
    return NULL;
}

uint64_t fat_find_dentry(struct superblock* sb, uint64_t parent_inode_index, const char *name, int* type)
{
    printk("fat_find_dentry (%i %s)\n", parent_inode_index, name);
    struct fat_instance* inst = sb->fs_info;

    // Найти и считать direntry
    struct fat_direntry dir_direntry;
    fat_read_dentry(inst, parent_inode_index, &dir_direntry, NULL);

    size_t namelen = strlen(name);
    // Буфер под кластер
    uint32_t cluster_sz = inst->bpb->sectors_per_cluster * inst->bytes_per_sector;
    char* cluster_buffer = kmalloc(cluster_sz);
    // Текущий кластер
    uint32_t current_cluster = FAT_DIRENTRY_GET_CLUSTER(dir_direntry);

    while (current_cluster != FAT_EOC)
    {
        fat_read_cluster(inst, current_cluster, cluster_buffer);
        uint32_t pos = 0;

        struct fat_direntry*    d_ptr;
        struct fat_lfn*         d_lfn;

        while (pos < cluster_sz) 
        {
            d_ptr = &cluster_buffer[pos];

            if (d_ptr->attr == FILE_ATTR_LFN) {
                d_lfn = d_ptr;
                pos += sizeof(struct fat_lfn);
                // TODO: fill LFN name
                continue;
            } else {
                printk("SFN %s\n", d_ptr->name);
                //if (strcmp(d_ptr->name, name))
            }

            break;
        }

        current_cluster = fat_get_next_cluster(inst, current_cluster);
    }

    kfree(cluster_buffer);
    return WRONG_INODE_INDEX;
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