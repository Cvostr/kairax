#include "../vfs/filesystems.h"
#include "fat.h"
#include "stdio.h"
#include "mem/kheap.h"
#include "kairax/stdio.h"
#include "string.h"
#include "kairax/kstdlib.h"

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

    // Первый сектор корневой директории (FAT12 FAT16)
    instance->root_dir_sector = bpb->reserved_sector_count + (bpb->fats_count * instance->fat_size);
    // Размер корневой директории в секторах (FAT12 FAT16)
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
    uint64_t ino_num = ((uint64_t)instance->root_dir_sector) << 32 | 0;

    struct fat_direntry root_direntry;
    fat_read_dentry(instance, ino_num, &root_direntry, NULL);
    printk("FAT: Root dentry name %s sector %i sectors %i\n", root_direntry.name, instance->root_dir_sector, instance->root_dir_sectors);

    struct inode* result = new_vfs_inode();
    instance->vfs_root_inode = result;
    result->inode = ino_num;
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

uint32_t fat_get_first_cluster_idx(struct fat_instance* inst, struct fat_direntry* entry)
{
    return ((((uint32_t) entry->first_cluster_hi) << 16) | entry->first_cluster_lo);
}

#define FAT_DIR_LAST_CLUSTER 100000
int fat_read_directory_cluster(struct fat_instance* inst, struct fat_direntry* direntry, int is_root, uint32_t index, char* buffer)
{
    uint32_t current_cluster = fat_get_first_cluster_idx(inst, direntry);
    uint32_t cluster_sz = inst->bpb->sectors_per_cluster * inst->bytes_per_sector;

    if (is_root == TRUE)
    {
        // Жуткий костыль для корневой директории
        switch (inst->fs_type) {
            case FS_FAT12:
            case FS_FAT16:
                // для Fat12 и Fat16 корневая директория состоит из секторов, а не из кластеров. Пиздец....

                if (index >= inst->root_dir_sectors)
                {
                    return FAT_DIR_LAST_CLUSTER;
                }

                current_cluster = inst->root_dir_sector + index;
                fat_read_sector(inst, current_cluster, 1, buffer);
                return 0;
                break;
            case FS_FAT32:
                // Для Fat32 кластер корневой директории задан в заголовке
                current_cluster = inst->bpb->ext_32.root_cluster;
        }
    }

    // Вычислим номер нужного кластера
    for (uint32_t i = 0; i < index; i ++)
    {
        current_cluster = fat_get_next_cluster(inst, current_cluster);
    }

    if (current_cluster == FAT_EOC)
    {
        return FAT_DIR_LAST_CLUSTER;
    }

    return fat_read_cluster(inst, current_cluster, buffer);
}

void fat_nseize_str(uint16_t* in, size_t len, char* out)
{
    size_t i = 0;

    for (i; i < len; i ++)
    {
        uint16_t val = in[i];
        out[i] = (char) val;
    }
}

void fat_apply_lfn(char* namebuffer, struct fat_lfn* lfn)
{
    uint8_t order = lfn->order & (~0x40); 
    uint32_t lfn_pos = (order - 1) * 13;

    //printk("LFN lfn_pos=%i order=%i\n", lfn_pos, order);

    // TODO: Unicode?
    fat_nseize_str(lfn->name_0, sizeof(lfn->name_0) / 2, namebuffer + lfn_pos);
    lfn_pos += sizeof(lfn->name_0) / 2;
    fat_nseize_str(lfn->name_1, sizeof(lfn->name_1) / 2, namebuffer + lfn_pos);
    lfn_pos += sizeof(lfn->name_1) / 2;
    fat_nseize_str(lfn->name_2, sizeof(lfn->name_2) / 2, namebuffer + lfn_pos);
    lfn_pos += sizeof(lfn->name_2) / 2;

    if ((lfn->order & 0x40) == 0x40)
    {
        namebuffer[lfn_pos] = '\0';
    }
}

uint64_t fat_find_dentry(struct superblock* sb, struct inode* parent_inode, const char *name, int* type)
{
    struct fat_instance* inst = sb->fs_info;
    int is_root_directory = parent_inode == inst->vfs_root_inode;
    uint64_t result_inode = WRONG_INODE_INDEX;

    printk("fat_find_dentry (ino %i (root = %i) name %s)\n", parent_inode->inode, is_root_directory, name);

    // Найти и считать direntry
    struct fat_direntry dir_direntry;
    fat_read_dentry(inst, parent_inode->inode, &dir_direntry, NULL);

    size_t namelen = strlen(name);
    // Буфер под кластер
    uint32_t cluster_sz = inst->bpb->sectors_per_cluster * inst->bytes_per_sector;

    if (is_root_directory) {
        // для Fat12 и Fat16 корневая директория состоит из секторов, а не из кластеров. Пиздец....
        if (inst->fs_type == FS_FAT12 || inst->fs_type == FS_FAT16)
            cluster_sz = inst->bytes_per_sector;
    }

    char* checking_name = kmalloc(FAT_LFN_MAX_SZ + 1);
    char* cluster_buffer = kmalloc(cluster_sz);
    uint32_t current_cluster_idx = 0;

    checking_name[0] = '\0';

    while (fat_read_directory_cluster(inst, &dir_direntry, is_root_directory, current_cluster_idx ++, cluster_buffer) != FAT_DIR_LAST_CLUSTER)
    {
        uint32_t pos = 0;

        struct fat_direntry*    d_ptr;
        struct fat_lfn*         d_lfn;

        while (pos < cluster_sz) 
        {
            d_ptr = &cluster_buffer[pos];

            if (d_ptr->attr == FILE_ATTR_LFN) {
                // Встретилась запись LFN
                d_lfn = d_ptr;
                fat_apply_lfn(checking_name, d_lfn);
                pos += sizeof(struct fat_lfn);
                continue;
            } else {

                if (strlen(checking_name) == 0)
                {
                    // Если не было LFN, используем SFN
                    strncpy(checking_name, d_ptr->name, sizeof(d_ptr->name));

                    uint8_t i;
                    // Сначала удалить все пробелы справа (1 пробел вначале лучше оставить)
                    for (i = 10; (i > 0) && (checking_name[i] == ' '); i --)
                        checking_name[i] = '\0';

                    // TODO: удалить пробелы между именем и расширением

                    // Снизить регистр имени, если установлен соответствующий флаг
                    if (d_ptr->nt_reserved & FILE_NTRES_NAME_LOWERCASE) {
                        for (i = 0; i < 8; i ++)
                            checking_name[i] = tolower(checking_name[i]);
                    }
                    // Снизить регистр расширения, если установлен соответствующий флаг
                    if (d_ptr->nt_reserved & FILE_NTRES_EXT_LOWERCASE) {
                        for (i = 8; i < 11; i ++)
                            checking_name[i] = tolower(checking_name[i]);
                    }
                }

                // Пустые dentry - не тратим время, сразу выходим 
                if (checking_name[0] == 0)
                {
                    result_inode = WRONG_INODE_INDEX;
                    goto exit;
                }

                // dentry не удалена и не является меткой тома
                if ((d_ptr->name[0] != 0xE5) && (d_ptr->attr & FILE_ATTR_VOLUME_ID) == 0)
                {
                    //printk("Name '%s' Attr %x\n", checking_name, d_ptr->attr);
                    if (strcmp(checking_name, name) == 0)
                    {
                        int is_dir = ((d_ptr->attr & FILE_ATTR_DIRECTORY) == FILE_ATTR_DIRECTORY);
                        *type = is_dir ? DT_DIR : DT_REG; 
                        printk("FOUND");
                        goto exit;
                    }
                }

                // Сброс буфера LFN
                checking_name[0] = '\0';
                // Увеличить смещение
                pos += sizeof(struct fat_direntry);

                continue;
            }
        }
    }

exit:
    kfree(checking_name);
    kfree(cluster_buffer);
    return result_inode;
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