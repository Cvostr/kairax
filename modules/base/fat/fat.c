#include "fs/vfs/filesystems.h"
#include "fat.h"
#include "kairax/stdio.h"
#include "kairax/stdio.h"
#include "kairax/string.h"
#include "kairax/kstdlib.h"

#include "module.h"
#include "functions.h"

struct super_operations fat_sb_ops;

// ---- file operations
struct file_operations fat_dir_ops;
struct file_operations fat_file_ops;

// ---- inode operations
struct inode_operations fat_file_inode_ops;
struct inode_operations fat_dir_inode_ops;

#define FAT_MOUNT_LOG

#define KFREE_SAFE(x) if (x) {kfree(x);}

int fat_init(void)
{
    filesystem_t* fatfs = new_filesystem();
    fatfs->name = "fat";
    fatfs->mount = fat_mount;
    fatfs->unmount = fat_unmount;

    filesystem_register(fatfs);

    fat_sb_ops.read_inode = fat_read_node;
    fat_sb_ops.stat = fat_statfs;
    fat_sb_ops.find_dentry = fat_find_dentry;

    memset(&fat_dir_ops, 0, sizeof(struct file_operations));
    fat_dir_ops.readdir = fat_file_readdir;

    memset(&fat_file_ops, 0, sizeof(struct file_operations));
    fat_file_ops.read = fat_file_read;
    fat_file_ops.write = fat_file_write;

    fat_file_inode_ops.truncate = fat_truncate;

    fat_dir_inode_ops.mkfile = fat_mkfile;
    fat_dir_inode_ops.mkdir = fat_mkdir;

    return 0;
}

void fat_exit(void)
{
    // TODO deregister fs
}

DEFINE_MODULE("fat", fat_init, fat_exit)

int fat_read_sector(struct fat_instance* inst, uint64_t first_sector, uint64_t sectors, char* buffer)
{
    if (inst->vfs_sb->detached == TRUE)
        return -EIO;

    if (first_sector >= inst->sectors_count)
    {   
        printk("FAT: Sector %i is outside of disk\n", first_sector);
        return -E2BIG;
    }

    uint32_t    bps = inst->bytes_per_sector;
    uint64_t    start_lba = first_sector * bps / 512;
    uint64_t    lba_count = sectors * bps / 512;
    
    return partition_read(inst->partition, start_lba, lba_count, buffer);
}

int fat_write_sector(struct fat_instance* inst, uint64_t first_sector, uint64_t sectors, char* buffer)
{
    if (inst->vfs_sb->detached == TRUE)
        return -EIO;

    if (first_sector >= inst->sectors_count)
    {   
        printk("FAT: Sector %i is outside of disk\n", first_sector);
        return -E2BIG;
    }

    uint32_t    bps = inst->bytes_per_sector;
    uint64_t    start_lba = first_sector * bps / 512;
    uint64_t    lba_count = sectors * bps / 512;
    
    return partition_write(inst->partition, start_lba, lba_count, buffer);
}

int fat_read_cluster(struct fat_instance* inst, uint32_t cluster, char* buffer)
{
    if (inst->vfs_sb->detached == TRUE)
        return -EIO;

    if (cluster < 2 || cluster >= inst->total_clusters)
    {   
        printk("FAT: Cluster %i is outside of disk\n", cluster);
        return -E2BIG;
    }

    uint32_t    spc = inst->bpb->sectors_per_cluster;
    uint32_t    bps = inst->bytes_per_sector;
    uint32_t    bpc = spc * bps;

    // Перевод номера кластера в адрес сектора
    uint32_t    first_sector = ((cluster - 2) * spc) + inst->first_data_sector;

    uint64_t    start_lba = first_sector * bps / 512;
    uint64_t    lba_count = bpc / 512;

    return partition_read(inst->partition, start_lba, lba_count, buffer);
}

int fat_write_cluster(struct fat_instance* inst, uint32_t cluster, const char* buffer)
{
    if (inst->vfs_sb->detached == TRUE)
        return -EIO;
        
    if (cluster < 2 || cluster >= inst->total_clusters)
    {   
        printk("FAT: Cluster %i is outside of disk\n", cluster);
        return -E2BIG;
    }

    uint32_t    spc = inst->bpb->sectors_per_cluster;
    uint32_t    bps = inst->bytes_per_sector;
    uint32_t    bpc = spc * bps;

    // Перевод номера кластера в адрес сектора
    uint32_t    first_sector = ((cluster - 2) * spc) + inst->first_data_sector;

    uint64_t    start_lba = first_sector * bps / 512;
    uint64_t    lba_count = bpc / 512;

    return partition_write(inst->partition, start_lba, lba_count, buffer);
}

uint32_t fat_get_first_cluster_idx(struct fat_instance* inst, struct fat_direntry* entry)
{
    return ((((uint32_t) entry->first_cluster_hi) << 16) | entry->first_cluster_lo);
}

uint64_t fat_to_inode_num_sector(struct fat_instance* inst, uint32_t sector, uint32_t offset)
{
    uint32_t bps = inst->bytes_per_sector;

    sector += offset / bps;
    offset = offset % bps;

    return ((uint64_t) sector) << 32 | offset;
}

uint64_t fat_to_inode_num(struct fat_instance* inst, uint32_t cluster, uint32_t offset)
{
    uint32_t    spc = inst->bpb->sectors_per_cluster;
    uint32_t    first_sector = ((cluster - 2) * spc) + inst->first_data_sector;
    
    return ((uint64_t) first_sector) << 32 | offset;
}

uint32_t fat_get_directory_buffer_size(struct fat_instance* inst, int is_root)
{
    if (is_root) {
        // для Fat12 и Fat16 корневая директория состоит из секторов, а не из кластеров.
        if (inst->fs_type == FS_FAT12 || inst->fs_type == FS_FAT16)
            return inst->bytes_per_sector;
    }

    return inst->bytes_per_cluster;
}

int fat_is_eoc(struct fat_instance* inst, uint32_t cluster_id)
{
    switch (inst->fs_type)
    {
    case FS_FAT12:
        return (cluster_id >= FAT12_EOC_MIN && cluster_id <= FAT12_EOC_MAX);
    case FS_FAT16:
        return (cluster_id >= FAT16_EOC_MIN && cluster_id <= FAT16_EOC_MAX);
    case FS_FAT32:
        return (cluster_id >= FAT32_EOC_MIN && cluster_id <= FAT32_EOC_MAX);
    case FS_EXFAT:
        return cluster_id == EXFAT_EOC;
    default:
        return -1;
    }
}

time_t fat_date_to_epoch(struct fat_date* date, struct fat_time* time)
{
	struct tm datetime;
    memset(&datetime, 0, sizeof(datetime));
	datetime.tm_year  =  (date->year % 128) + 1980 - 1900;
	datetime.tm_mon = ((date->month - 1) % 12) + 1;
	datetime.tm_mday   = ((date->day - 1) % 31) + 1;

    if (time != NULL)
    {
        datetime.tm_hour   = (time->hour % 24);
        datetime.tm_min = (time->minute % 60);
        datetime.tm_sec = (time->second % 30) * 2;
    }

    return tm_to_epoch(&datetime);
}

void fat_epoch_to_datetime(time_t epoch, struct fat_date* date, struct fat_time* time)
{
    struct tm datetime;
    epoch_to_tm(&epoch, &datetime);

    // fat 1980 -> 2107, tm - 1900 +
    date->year = (datetime.tm_year - 80) % 128;
    date->month = datetime.tm_mon + 1;
    date->day = datetime.tm_mday;

    if (time) 
    {
        time->hour = datetime.tm_hour;
        time->minute = datetime.tm_min;
        time->second = datetime.tm_sec / 2;
    }
}

uint8_t fat_get_lfn_checksum(const uint8_t *sfn) 
{
    uint8_t checksum = 0;

    for (int i = 0; i < 11; i++) {
        checksum = ((checksum & 1) << 7) | (checksum >> 1);
        checksum += sfn[i];
    }

    return checksum;
}

void fat_toupper(uint8_t *str, size_t len)
{
    for (size_t i = 0; i < len; i ++)
    {
        uint8_t c = str[i];
        if ('a' <= c && c <= 'z')
    	    str[i] += 'A' - 'a';
    }
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

    uint64_t bsize = drive->drive->block_size;
    if (bsize < 512)
    {
        printk("FAT: Disk block size (%i) is lower than minimum required 512!\n");
        fat_free_instance(instance);
        return NULL;
    }

    struct fat_bpb* bpb = kmalloc(bsize); 
    instance->bpb = bpb;
    memset(instance->bpb, 0, bsize);

    // Считать BPB
    if (partition_read(drive, 0, 1, (char *) instance->bpb) != 0)
    {
        printk("FAT: Error reading BPB!\n");
        fat_free_instance(instance);
        return NULL;
    }

    uint8_t* jmp_op = instance->bpb->jump_op;
    int jmp_op_valid = (jmp_op[0] == 0xEB && jmp_op[2] == 0x90) || (jmp_op[0] == 0xE9);
    if (jmp_op_valid == FALSE)
    {
        printk("FAT: Incorrect JMP command!\n");
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

    // Байт в кластере
    instance->bytes_per_cluster = ((uint32_t) instance->bpb->sectors_per_cluster) * instance->bytes_per_sector;

    // Сохранить размер FAT (в секторах)
    instance->fat_size = (bpb->fat_size16 == 0) ? bpb->ext_32.fat_size : bpb->fat_size16;

    // Первый сектор корневой директории (FAT12 FAT16)
    instance->root_dir_sector = bpb->reserved_sector_count + (bpb->fats_count * instance->fat_size);
    // Размер корневой директории в секторах (FAT12 FAT16)
    instance->root_dir_sectors = ((bpb->root_entry_count * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;

    // максимум учитываем в работе первые два FATs
    instance->fats_count = MIN(instance->bpb->fats_count, 2);
    // сразу рассчитаем первые сектора для таблиц FAT
    for (uint32_t fat_i = 0; fat_i < instance->fats_count; fat_i ++) {
        instance->first_fats_sectors[fat_i] = bpb->reserved_sector_count + (fat_i * instance->fat_size);
    }

    instance->first_data_sector = bpb->reserved_sector_count + (bpb->fats_count * instance->fat_size) + instance->root_dir_sectors;

    // Количество секторов, отведенных под данные (вычитаем все FATs, зарезервированные и корневую директорию)
    instance->data_sectors = instance->sectors_count - (bpb->reserved_sector_count + (bpb->fats_count * instance->fat_size) + instance->root_dir_sectors);

    // Количество кластеров
    instance->total_clusters = instance->data_sectors / bpb->sectors_per_cluster;

    // Определить тип файловой системы
    if (bpb->bytes_per_sector == 0) {
        instance->fs_type = FS_EXFAT;
        // Размер для буфера FAT
        instance->fat_buffer_size = instance->bytes_per_sector;
        // пока не реализовано полноценно
        printk("exFAT is unsupported!\n");
        fat_free_instance(instance);
        return NULL;
    } else if(instance->total_clusters < 4085) {
        instance->fs_type = FS_FAT12;
        // Размер для буфера FAT
        instance->fat_buffer_size = instance->bytes_per_sector * 2;
        // значение EOC
        instance->eoc_value = FAT12_EOC_MAX;
        instance->bad_value = FAT12_BAD;
    } else if(instance->total_clusters < 65525) {
        instance->fs_type = FS_FAT16;
        // Размер для буфера FAT
        instance->fat_buffer_size = instance->bytes_per_sector;
        // значение EOC
        instance->eoc_value = FAT16_EOC_MAX;
        instance->bad_value = FAT16_BAD;
    } else {
        instance->fs_type = FS_FAT32;
        // Размер для буфера FAT
        instance->fat_buffer_size = instance->bytes_per_sector;
        // значение EOC
        instance->eoc_value = FAT32_EOC_MAX;
        instance->bad_value = FAT32_BAD;
        // Чтение структуры FSINFO32
        instance->fsinfo32 = kmalloc(bsize);
        if (partition_read(drive, bpb->ext_32.filesystem_info, 1, (char *) instance->fsinfo32) != 0)
        {
            printk("FAT: Error reading FSINFO32!\n");
            fat_free_instance(instance);
            return NULL;
        }
        // Проверка
        if (instance->fsinfo32->lead_signature != FSINFO32_SIGNATURE || instance->fsinfo32->trail_signature != FSINFO32_TRAIL_SIGNATURE)
        {
            printk("Fat32 FSINFO structure signatures incorrect!\n");
            fat_free_instance(instance);
            return NULL;
        }

        instance->free_clusters = instance->fsinfo32->last_free_cluster_count;
    }

    // Считать блоки имеет смысла только для FAT12 и 16. у FAT32 это уже записано в FSINFO
    if (instance->fs_type == FS_FAT16 || instance->fs_type == FS_FAT12)
    {
        // Вычислить свободные кластера
        if (fat_calc_free_clusters(instance, &instance->free_clusters))
        {
            printk("Error calculating free clusters!\n");
            fat_free_instance(instance);
            return NULL;
        }
    }

#ifdef FAT_MOUNT_LOG
    printk("FAT: type %i FATs %i FAT size %i sectors %i BPS %i Clusters %i Free %i SPC %i BPC %i FatBufferSz %i\n", 
        instance->fs_type, 
        bpb->fats_count,
        instance->fat_size,
        instance->sectors_count,
        bpb->bytes_per_sector,
        instance->total_clusters,
        instance->free_clusters,
        bpb->sectors_per_cluster,
        instance->bytes_per_cluster,
        instance->fat_buffer_size);

    for (uint32_t fat_i = 0; fat_i < instance->fats_count; fat_i ++) {
        printk("FAT %i first sector %i\n", fat_i, instance->first_fats_sectors[fat_i]);
    }
#endif

    uint64_t ino_num = 0;
    switch (instance->fs_type)
    {
    case FS_FAT12:
    case FS_FAT16:
        ino_num = fat_to_inode_num_sector(instance, instance->root_dir_sector, 0);
        break;
    case FS_FAT32:
        ino_num = fat_to_inode_num(instance, bpb->ext_32.root_cluster, 0);
        break;
    }

    // создание VFS объекта для root inode
    struct inode* result = new_vfs_inode();
    instance->vfs_root_inode = result;
    result->inode = ino_num;
    result->sb = sb;
    result->uid = 0;
    result->gid = 0;
    result->size = 0;
    result->mode = INODE_TYPE_DIRECTORY | 0777;
    result->access_time.tv_sec = 0;
    result->create_time.tv_sec = 0;
    result->modify_time.tv_sec = 0;
    result->hard_links = 1;
    result->device = (dev_t) instance;
    result->file_ops = &fat_dir_ops;
    result->operations = &fat_dir_inode_ops;

    sb->fs_info = instance;
    sb->blocksize = bpb->bytes_per_sector;
    sb->operations = &fat_sb_ops;

    return result;
}

int fat_calc_free_clusters(struct fat_instance* inst, uint32_t *free_clusters)
{
    int rc;
    uint32_t next;
    uint32_t last_sector = 0;
    uint32_t counter = 0;
    char* fat_buffer = kmalloc(inst->fat_buffer_size);

    for (uint32_t i = 0; i < inst->total_clusters; i ++)
    {
        rc = fat_get_next_cluster_ex(inst, i, fat_buffer, &last_sector, &next);
        if (rc != 0)
            return rc;

        if (next == 0) counter ++;
    }

    kfree(fat_buffer);
    *free_clusters = counter;

    return 0;
}

int fat_read_dentry(struct fat_instance* inst, uint64_t inode_num, struct fat_direntry* dentry) 
{
    if (inode_num == WRONG_INODE_INDEX)
    {
        return -ENOENT;
    }

    uint32_t sector = inode_num >> 32;
    uint32_t offset = inode_num & UINT32_MAX;

    if (offset % 32 != 0)
    {
        return -ENOENT;
    }

    char* sector_buffer = kmalloc(inst->bytes_per_sector);
    if (sector_buffer == NULL)
    {
        return -ENOMEM;
    }

    int rc = fat_read_sector(inst, sector, 1, sector_buffer);
    if (rc != 0)
    {
        kfree(sector_buffer);
        return rc;
    }

    struct fat_direntry* d_ptr = (struct fat_direntry*) (sector_buffer + offset);

    if ((d_ptr->name[0] == 0) || (d_ptr->name[0] == 0xE5))
    {
        kfree(sector_buffer);
        return -ENOENT;
    }

    memcpy(dentry, d_ptr, sizeof(struct fat_direntry));
    kfree(sector_buffer);
    return 0;
}

int fat_write_dentry(struct fat_instance* inst, uint64_t inode_num, struct fat_direntry* dentry) 
{
    if (inode_num == WRONG_INODE_INDEX)
    {
        return -ENOENT;
    }

    uint32_t sector = inode_num >> 32;
    uint32_t offset = inode_num & UINT32_MAX;

    if (offset % 32 != 0)
    {
        return -ENOENT;
    }

    char* sector_buffer = kmalloc(inst->bytes_per_sector);
    if (sector_buffer == NULL)
    {
        return -ENOMEM;
    }

    int rc = fat_read_sector(inst, sector, 1, sector_buffer);
    if (rc != 0)
    {
        kfree(sector_buffer);
        return rc;
    }

    struct fat_direntry* d_ptr = (struct fat_direntry*) (sector_buffer + offset);

    memcpy(d_ptr, dentry, sizeof(struct fat_direntry));

    rc = fat_write_sector(inst, sector, 1, sector_buffer);
    if (rc != 0)
    {
        kfree(sector_buffer);
        return rc;
    }

    kfree(sector_buffer);
    return 0;
}

int fat_get_next_cluster(struct fat_instance* inst, uint32_t cluster, uint32_t *next_cluster)
{
    int rc;
    uint32_t sector;
    // Выделить буфер под FAT
    char* fat_buffer = kmalloc(inst->fat_buffer_size);
    rc = fat_get_next_cluster_ex(inst, cluster, fat_buffer, &sector, next_cluster);
    KFREE_SAFE(fat_buffer);

    return rc;
}

int fat_get_next_cluster_ex(struct fat_instance* inst, uint32_t cluster, char* fat_buffer, uint32_t* last_sector, uint32_t *next_cluster)
{
    int rc;
    uint32_t result = 0;
    uint32_t fat_offset = 0;    // Смещение внутри таблицы FAT в байтах
    uint32_t fat_sector = 0;    // Номер сектора в таблице FAT (Смещение внутри таблицы FAT в секторах)
    uint32_t sector_offset = 0; // Смещение внутри сектора таблицы FAT

    uint32_t first_fat_sector = inst->first_fats_sectors[0];

    switch (inst->fs_type)
    {
        case FS_FAT12:
            fat_offset = cluster + (cluster / 2);  
            fat_sector = first_fat_sector + (fat_offset / inst->bytes_per_sector); 
            sector_offset = fat_offset % inst->bytes_per_sector;
            
            if (*last_sector != fat_sector)
                if ((rc = fat_read_sector(inst, fat_sector, 2, fat_buffer)) != 0)
                    return rc;
            
            result = *((uint16_t*) &fat_buffer[sector_offset]);
            result = (cluster & 1) ? result >> 4 : result & 0xfff;
            break;
        case FS_FAT16:
            fat_offset = cluster * sizeof(uint16_t);
            fat_sector = first_fat_sector + (fat_offset / inst->bytes_per_sector);
            sector_offset = fat_offset % inst->bytes_per_sector;

            if (*last_sector != fat_sector)
                if ((rc = fat_read_sector(inst, fat_sector, 1, fat_buffer)) != 0)
                    return rc;

            result = *((uint16_t*) &fat_buffer[sector_offset]);
            break;
        case FS_FAT32:
        case FS_EXFAT:
            fat_offset = cluster * sizeof(uint32_t);
            fat_sector = first_fat_sector + (fat_offset / inst->bytes_per_sector);
            sector_offset = fat_offset % inst->bytes_per_sector;

            if (*last_sector != fat_sector)
                if ((rc = fat_read_sector(inst, fat_sector, 1, fat_buffer)) != 0)
                    return rc;

            result = *((uint32_t*) &fat_buffer[sector_offset]);
            if (inst->fs_type == FS_FAT32)
            {
                result &= 0x0FFFFFFF;
            }
            break;
    }

    *next_cluster = result;
    *last_sector = fat_sector;

    return 0;
}

int fat_set_next_cluster(struct fat_instance* inst, uint32_t cluster, uint32_t next_cluster)
{
    int rc = 0;
    uint32_t fat_offset = 0;    // Смещение внутри таблицы FAT в байтах
    uint32_t fat_sector = 0;    // Номер сектора в таблице FAT (Смещение внутри таблицы FAT в секторах)
    uint32_t sector_offset = 0; // Смещение внутри сектора таблицы FAT

    char* fat_buffer = kmalloc(inst->fat_buffer_size);
    if (fat_buffer == NULL)
        return -ENOMEM;

    // запись надо выполнить в несколько FAT, если их несколько
    for (uint32_t fat_i = 0; fat_i < inst->fats_count; fat_i ++) 
    {
        uint32_t first_fat_sector = inst->first_fats_sectors[fat_i];
        switch (inst->fs_type)
        {
            case FS_FAT12:
                fat_offset = cluster + (cluster / 2);  
                fat_sector = first_fat_sector + (fat_offset / inst->bytes_per_sector); 
                sector_offset = fat_offset % inst->bytes_per_sector;
            
                if ((rc = fat_read_sector(inst, fat_sector, 2, fat_buffer)) != 0)
                    goto exit;
                
                //result = *((uint16_t*) &fat_buffer[sector_offset]);
                //result = (cluster & 1) ? result >> 4 : result & 0xfff;
                rc = -ENOTSUP;
                break;
            case FS_FAT16:
                fat_offset = cluster * sizeof(uint16_t);
                fat_sector = first_fat_sector + (fat_offset / inst->bytes_per_sector);
                sector_offset = fat_offset % inst->bytes_per_sector;

                if ((rc = fat_read_sector(inst, fat_sector, 1, fat_buffer)) != 0)
                    goto exit;

                *((uint16_t*) &fat_buffer[sector_offset]) = (uint16_t) next_cluster;

                if ((rc = fat_write_sector(inst, fat_sector, 1, fat_buffer)) != 0)
                    goto exit;

                break;
            case FS_FAT32:
            case FS_EXFAT:
                fat_offset = cluster * sizeof(uint32_t);
                fat_sector = first_fat_sector + (fat_offset / inst->bytes_per_sector);
                sector_offset = fat_offset % inst->bytes_per_sector;

                if ((rc = fat_read_sector(inst, fat_sector, 1, fat_buffer)) != 0)
                    goto exit;

                if (inst->fs_type == FS_FAT32) {
                    next_cluster &= 0x0FFFFFFF;
                }

                *((uint32_t*) &fat_buffer[sector_offset]) = next_cluster;

                if ((rc = fat_write_sector(inst, fat_sector, 1, fat_buffer)) != 0)
                    goto exit;

                break;
        }
    }

exit:
    kfree(fat_buffer);
    return rc;
}

int fat_alloc_cluster(struct fat_instance* inst, uint32_t *cluster_id)
{
    int rc = 0;
    
    // Выделить буфер под FAT
    char* fat_buffer = kmalloc(inst->fat_buffer_size);
    if (fat_buffer == NULL)
        return -ENOMEM;

    uint32_t next;
    uint32_t last_sector = 0;
    int found = FALSE;

    for (uint32_t i = 2; i < inst->total_clusters; i ++)
    {
        rc = fat_get_next_cluster_ex(inst, i, fat_buffer, &last_sector, &next);
        if (rc != 0)
            return rc;

        // Если кластер свободен
        if (next == 0)
        {
            found = TRUE;
            *cluster_id = i;
            // Терминируем кластер
            if ((rc = fat_set_next_cluster(inst, i, inst->eoc_value)) != 0)
                goto exit;

            // Если есть структура FSinfo32, то в ней надо уменьшить количество свободных кластеров
            if (inst->fsinfo32) {
                inst->fsinfo32->last_free_cluster_count --;
            }

            break;
        }
    }

    // не нашли ни одного свободного кластера - значит места на диске не осталось
    if (found == FALSE)
        rc = -ENOSPC;

exit:
    kfree(fat_buffer);
    return rc;
}

int fat_free_cluster(struct fat_instance* inst, uint32_t cluster_id)
{
    int rc = fat_set_next_cluster(inst, cluster_id, 0);

    // Если есть структура FSinfo32, то в ней надо увеличить количество свободных кластеров
    if (rc == 0 && inst->fsinfo32) {
        inst->fsinfo32->last_free_cluster_count ++;
    }

    return rc;
}

struct inode* fat_read_node(struct superblock* sb, uint64_t ino_num)
{
    struct fat_instance* inst = sb->fs_info;
    //printk("fat_read_node (%i)\n", ino_num);

    struct fat_direntry direntry;
    if (fat_read_dentry(inst, ino_num, &direntry) != 0)
    {
        return NULL;
    }

    int is_dir = ((direntry.attr & FILE_ATTR_DIRECTORY) == FILE_ATTR_DIRECTORY);

    struct inode* result = new_vfs_inode();
    result->inode = ino_num;
    result->sb = sb;
    result->uid = 0;
    result->gid = 0;
    result->size = direntry.file_size;
    result->mode = is_dir ? INODE_TYPE_DIRECTORY : INODE_TYPE_FILE;
    result->mode |= 0777;   // в FAT нет ACL
    result->access_time.tv_sec = fat_date_to_epoch(&direntry.last_access_date, NULL);
    result->modify_time.tv_sec = fat_date_to_epoch(&direntry.write_date, &direntry.write_time);
    result->create_time.tv_sec = fat_date_to_epoch(&direntry.creat_date, &direntry.creat_time);
    result->hard_links = 1;
    result->device = (dev_t) inst;

    if (is_dir) {
        result->file_ops = &fat_dir_ops;
        result->operations = &fat_dir_inode_ops;
    } else {
        result->file_ops = &fat_file_ops;
        result->operations = &fat_file_inode_ops;
    }

    return result;
}

#define FAT_DIR_LAST_CLUSTER 100000
int fat_read_directory_cluster( struct fat_instance* inst, 
                                struct fat_direntry* direntry,
                                int is_root,
                                uint32_t index,
                                char* buffer,
                                uint64_t* first_sector,
                                uint32_t *cluster_id)
{
    int rc;
    uint32_t current_cluster = 0;

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
                // Абсолютный номер сектора
                current_cluster = inst->root_dir_sector + index;
                // Считать сектор
                if ((rc = fat_read_sector(inst, current_cluster, 1, buffer)) != 0) {
                    return rc;
                }

                *first_sector = current_cluster;
                *cluster_id = current_cluster;

                return 0;
            case FS_FAT32:
                // Для Fat32 кластер корневой директории задан в заголовке
                current_cluster = inst->bpb->ext_32.root_cluster;
        }
    } else {
        // Если это не корневая папка - берем первый кластер из dentry
        current_cluster = fat_get_first_cluster_idx(inst, direntry);
        // папка не имеет ни одного кластера
        if (current_cluster == 0) {
            return FAT_DIR_LAST_CLUSTER;
        }
    }

    // Вычислим номер нужного кластера
    for (uint32_t i = 0; i < index; i ++)
    {
        if ((rc = fat_get_next_cluster(inst, current_cluster, &current_cluster)) != 0)
            return rc;
    }

    if (fat_is_eoc(inst, current_cluster))
    {
        return FAT_DIR_LAST_CLUSTER;
    }

    *cluster_id = current_cluster;

    // Переводим кластер в сектор
    uint32_t    spc = inst->bpb->sectors_per_cluster;
    *first_sector = ((current_cluster - 2) * spc) + inst->first_data_sector;

    return fat_read_cluster(inst, current_cluster, buffer);
}

int fat_write_directory_cluster(struct fat_instance* inst, int is_root, uint32_t cluster_id, const char *cluster_data)
{
    int is_fat12_16 = (inst->fs_type == FS_FAT12) || (inst->fs_type == FS_FAT16);
    if (is_root && is_fat12_16) {
        return fat_write_sector(inst, cluster_id, 1, cluster_data);
    }

    return fat_write_cluster(inst, cluster_id, cluster_data);
}

void fat_apply_lfn(uint16_t *namebuffer16, char* namebuffer, struct fat_lfn* lfn)
{
    uint8_t order = lfn->order & (~0x40); 

    if (order < 1 || order > 20)
        return;

    uint32_t lfn_pos = (order - 1) * 13;
    size_t utf8_len = 0;

    // для удобства соберем фрагменты строки воедино
    uint16_t *utf16_str = &namebuffer16[lfn_pos];
    memcpy(utf16_str, lfn->name_0, sizeof(lfn->name_0));
    memcpy(&utf16_str[5], lfn->name_1, sizeof(lfn->name_1));
    memcpy(&utf16_str[11], lfn->name_2, sizeof(lfn->name_2));

    if (order == 1) {
        // конвертация в родной для системы UTF-8
        str_utf16_to_utf8(namebuffer, namebuffer16, FAT_LFN_MAX_SZ, FAT_LFN_MAX_SZ, &utf8_len);
    }
}

void fat_normalize_sfn(struct fat_direntry* direntry, char* sfname)
{
    uint8_t i;
    // Сначала удалить все пробелы справа (1 пробел вначале лучше оставить)
    for (i = 10; (i > 0) && (sfname[i] == ' '); i --)
        sfname[i] = '\0';

    // Этот ядерный алгоритм удаляет пробелы между именем и расширением ('file    txt' -> 'file.txt')
    char* first_space = strchr(sfname, ' ');
    if (first_space != NULL)
    {
        char* last_space = strrchr(sfname, ' ');
        // Сразу меняем первый пробел на точку
        *first_space = '.';
        // Сдвигаем обоих
        first_space++;
        last_space++;
        
        size_t delta = last_space - first_space;
        if (delta > 0)
        {
            size_t copy_amount = strlen(last_space) + 1;
            for (i = 0; i < copy_amount; i++)
                first_space[i] = last_space[i];
        }    
    }
    else
    {
        // пробелов нет, значит имя и расширение слеплены вместе
        // например, для $RECYCLE.BIN будет такое : $RECYCLEBIN
        // надо разъединить и поставить точку

        // сдвигаем расширение на 1 символ вперед
        for (i = strlen(sfname); i >= 8; i --)
            sfname[i + 1] = sfname[i];

        // Если у файла есть расширение (длина имени больше 8)
        // ставим точку на освободившуюся позицию 8
        if (strlen(sfname) > 8)
            sfname[8] = '.';
    }

    // Снизить регистр имени, если установлен соответствующий флаг
    if (direntry->nt_reserved & FILE_NTRES_NAME_LOWERCASE) {
        for (i = 0; i < 8; i ++)
            sfname[i] = tolower(sfname[i]);
    }
    // Снизить регистр расширения, если установлен соответствующий флаг
    if (direntry->nt_reserved & FILE_NTRES_EXT_LOWERCASE) {
        for (i = 8; i < 11; i ++)
            sfname[i] = tolower(sfname[i]);
    }
}

uint64_t fat_find_dentry(struct superblock* sb, struct inode* parent_inode, const char *name, int* type) 
{
    int rc;
    struct fat_instance* inst = sb->fs_info;

    // имя для проверки в UTF-16
    uint16_t *namebuffer16 = NULL;
    // имя для проверки в UTF-8
    char *checking_name = NULL;
    // буфер под кластер
    char *cluster_buffer = NULL;

    int is_root_directory = parent_inode == inst->vfs_root_inode;
    uint64_t result_inode = WRONG_INODE_INDEX;

    // Найти и считать direntry
    struct fat_direntry dir_direntry;
    if (is_root_directory == FALSE) 
    {
        // Читаем только если директория не корневая, так как dentry корневой директории не существует
        if (fat_read_dentry(inst, parent_inode->inode, &dir_direntry) != 0) {
            return WRONG_INODE_INDEX;
        }
    }
    
    // Буфер под кластер
    uint32_t cluster_sz = fat_get_directory_buffer_size(inst, is_root_directory);

    // Буфер под имя
    checking_name = kmalloc(FAT_LFN_MAX_SZ + 1);
    if (checking_name == NULL) {
        result_inode = WRONG_INODE_INDEX;
        goto exit;
    }

    // Буфер под имя UTF-16
    namebuffer16 = kmalloc(FAT_LFN_MAX_SZ * sizeof(uint16_t));
    if (namebuffer16 == NULL) {
        result_inode = WRONG_INODE_INDEX;
        goto exit;
    }
    
    // буфер под кластер
    cluster_buffer = kmalloc(cluster_sz);
    if (cluster_buffer == NULL) {
        result_inode = WRONG_INODE_INDEX;
        goto exit;
    }

    uint32_t current_cluster_idx = 0;
    uint32_t current_cluster_id = 0;
    uint64_t base_sector = 0;

    checking_name[0] = '\0';

    while (1)
    {
        uint32_t pos = 0;
        struct fat_direntry*    d_ptr;

        // считать следующий кластер директории
        rc = fat_read_directory_cluster(inst, &dir_direntry, is_root_directory, current_cluster_idx ++, cluster_buffer, &base_sector, &current_cluster_id);
        if (rc == FAT_DIR_LAST_CLUSTER)
            // считаны все кластера директории
            break;
        else if (rc != 0) {
            // произошла критическая ошибка чтения
            result_inode = WRONG_INODE_INDEX;
            goto exit;
        }

        while (pos < cluster_sz) 
        {
            d_ptr = (struct fat_direntry*) &cluster_buffer[pos];

            if (d_ptr->attr == FILE_ATTR_LFN) {
                // Встретилась запись LFN
                fat_apply_lfn(namebuffer16, checking_name, (struct fat_lfn*) d_ptr);
                pos += sizeof(struct fat_lfn);
            } 
            else 
            {
                if (strlen(checking_name) == 0)
                {
                    // Если не было LFN, используем SFN
                    strncpy(checking_name, d_ptr->name, sizeof(d_ptr->name));
                    // нормализовать SFN имя (удаление пробелов, регистр)
                    fat_normalize_sfn(d_ptr, checking_name);
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
                    if (strcmp(checking_name, name) == 0)
                    {
                        int is_dir = ((d_ptr->attr & FILE_ATTR_DIRECTORY) == FILE_ATTR_DIRECTORY);
                        if (type != NULL)
                            *type = is_dir ? DT_DIR : DT_REG; 
                        result_inode = fat_to_inode_num_sector(inst, base_sector, pos);
                        goto exit;
                    }
                }

                // Сброс буфера LFN
                checking_name[0] = '\0';
                // Увеличить смещение
                pos += sizeof(struct fat_direntry);
            }
        }
    }

exit:
    KFREE_SAFE(checking_name)
    KFREE_SAFE(namebuffer16)
    KFREE_SAFE(cluster_buffer)
    return result_inode;
}

struct dirent* fat_file_readdir(struct file* dir, uint32_t index)
{
    int rc;
    struct inode* vfs_inode = dir->inode;
    struct superblock* sb = vfs_inode->sb;
    struct fat_instance* inst = sb->fs_info;

    struct dirent* result = NULL;
    int is_root_directory = vfs_inode == inst->vfs_root_inode;
    
    // имя для проверки в UTF-16
    uint16_t *namebuffer16 = NULL;
    // буфер под кластер
    char *cluster_buffer = NULL;
    // имя для проверки в UTF-8
    char *checking_name = NULL;

    // Считать direntry
    struct fat_direntry direntry;
    if (is_root_directory == FALSE) 
    {
        // Нет необходимости делать это, так как dentry корневой директории не существует
        if (fat_read_dentry(inst, vfs_inode->inode, &direntry) != 0) 
        {
            return NULL;
        }
    }

    // Буфер под кластер
    uint32_t cluster_sz = fat_get_directory_buffer_size(inst, is_root_directory);
    // буфер под кластер
    cluster_buffer = kmalloc(cluster_sz);
    if (cluster_buffer == NULL) {
        goto exit;
    }

    // Буфер под имя
    checking_name = kmalloc(FAT_LFN_MAX_SZ + 1);
    if (checking_name == NULL) {
        goto exit;
    }

    // Буфер под имя UTF-16
    namebuffer16 = kmalloc(FAT_LFN_MAX_SZ * sizeof(uint16_t));
    if (namebuffer16 == NULL) {
        goto exit;
    }

    checking_name[0] = '\0';

    uint32_t current_cluster_idx = 0;
    uint32_t current_cluster_id = 0;
    uint64_t base_sector = 0;
    uint32_t dentry_iter = 0;
    struct fat_direntry*    d_ptr;
    
    while (1)
    {
        uint32_t pos = 0;

        // считать следующий кластер директории
        rc = fat_read_directory_cluster(inst, &direntry, is_root_directory, current_cluster_idx ++, cluster_buffer, &base_sector, &current_cluster_id);
        if (rc == FAT_DIR_LAST_CLUSTER)
            // считаны все кластера директории
            break;
        else if (rc != 0) {
            // произошла критическая ошибка чтения
            result = NULL;
            goto exit;
        }

        while (pos < cluster_sz) 
        {
            d_ptr = (struct fat_direntry*) &cluster_buffer[pos];

            if (d_ptr->attr == FILE_ATTR_LFN) 
            {
                // Встретилась запись LFN
                fat_apply_lfn(namebuffer16, checking_name, (struct fat_lfn*) d_ptr);
                pos += sizeof(struct fat_lfn);
            } 
            else
            {
                if (strlen(checking_name) == 0)
                {
                    // Если не было LFN, используем SFN
                    strncpy(checking_name, d_ptr->name, sizeof(d_ptr->name));
                    // нормализовать SFN имя (удаление пробелов, регистр)
                    fat_normalize_sfn(d_ptr, checking_name);
                }

                // Пустые dentry - не тратим время, сразу выходим 
                if (checking_name[0] == 0)
                {
                    goto exit;
                }

                // dentry не удалена и не является меткой тома
                if ((d_ptr->name[0] != 0xE5) && (d_ptr->attr & FILE_ATTR_VOLUME_ID) == 0)
                {
                    if (dentry_iter == index)
                    {
                        int is_dir = ((d_ptr->attr & FILE_ATTR_DIRECTORY) == FILE_ATTR_DIRECTORY);

                        result = new_vfs_dirent();
                        result->inode = fat_to_inode_num_sector(inst, base_sector, pos);
                        strncpy(result->name, checking_name, MAX_DIRENT_NAME_LEN - 1);
                        result->type = is_dir ? DT_DIR : DT_REG;

                        goto exit;
                    }

                    dentry_iter++;
                }

                // Сброс буфера LFN
                checking_name[0] = '\0';
                // Увеличить смещение
                pos += sizeof(struct fat_direntry);
            }
        }
    }

exit:
    KFREE_SAFE(checking_name)
    KFREE_SAFE(namebuffer16)
    KFREE_SAFE(cluster_buffer)
    return result;
}

ssize_t fat_file_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct inode* vfs_inode = file->inode;
    struct fat_instance* inst = vfs_inode->sb->fs_info;

    //printk("Fat read inode %i offset %i count %i\n", vfs_inode->inode, offset, count);

    struct fat_direntry direntry;
    int rc = fat_read_dentry(inst, vfs_inode->inode, &direntry);
    if (rc != 0)
    {
        return rc;
    }

    if (direntry.file_size < offset)
    {
        // Смещение больше размера файла
        return 0;
    }

    // Ограничение по размеру файла
    if (direntry.file_size < offset + count)
    {
        count = direntry.file_size - offset;
    }

    if (count == 0)
    {
        // Нечего читать - выходим
        return 0;
    }

    // Буфер под кластер
    uint32_t cluster_sz = inst->bytes_per_cluster;
    char* cluster_buffer = kmalloc(cluster_sz);
    if (cluster_buffer == NULL)
    {
        return -ENOMEM;
    }

    // Выделим буфер под FAT таблицу для переходов по кластерам
    // Чтобы постоянно не перевыделять эту память в fat_get_next_cluster() 
    char* fat_buffer = kmalloc(inst->fat_buffer_size);
    if (fat_buffer == NULL)
    {
        kfree(cluster_buffer);
        return -ENOMEM;
    }

    // Текущий кластер
    uint32_t current_cluster = fat_get_first_cluster_idx(inst, &direntry);
    uint32_t current_cluster_num = 0;

    // порядковый номер кластера, с которого надо начать читать с учетом смещения
    uint32_t first_cluster = offset / cluster_sz;
    // Смещение внутри первого кластера, с которого надо начать читать
    uint32_t cluster_offset = offset % cluster_sz;

    uint32_t last_sector = 0;
    ssize_t readed = 0;

    while (fat_is_eoc(inst, current_cluster) == FALSE && (count - readed) > 0)
    {
        if (current_cluster_num >= first_cluster)
        {
            rc = fat_read_cluster(inst, current_cluster, cluster_buffer);
            if (rc != 0)
            {
                readed = rc;
                goto exit;   
            }

            ssize_t available_data = MIN(count - readed, cluster_sz - cluster_offset);
            memcpy(buffer + readed, cluster_buffer + cluster_offset, available_data);
            cluster_offset = 0;
            readed += available_data;
        }

        if ((count - readed) <= 0)
        {
            break;
        }

        // Получить номер следующего кластера
        rc = fat_get_next_cluster_ex(inst, current_cluster, fat_buffer, &last_sector, &current_cluster);
        if (rc != 0)
        {
            readed = rc;
            goto exit;
        }


        current_cluster_num ++;
    }

    // Сдвинуть
    file->pos += readed;

exit:
    kfree(fat_buffer);
    kfree(cluster_buffer);
    return readed;
}

ssize_t fat_file_write(struct file* file, const char* buffer, size_t count, loff_t offset)
{
    struct inode* vfs_inode = file->inode;
    struct fat_instance* inst = vfs_inode->sb->fs_info;

    struct fat_direntry direntry;
    int rc = fat_read_dentry(inst, vfs_inode->inode, &direntry);
    if (rc != 0)
    {
        return rc;
    }

    if (count == 0)
    {
        // Нечего записывать - выходим
        return 0;
    }

    // Буфер под кластер
    uint32_t cluster_sz = inst->bytes_per_cluster;
    char* cluster_buffer = kmalloc(cluster_sz);
    if (cluster_buffer == NULL)
    {
        return -ENOMEM;
    }

    // Текущий кластер (номер в FAT)
    uint32_t current_cluster = fat_get_first_cluster_idx(inst, &direntry);
    uint32_t next_cluster;

    // порядковый номер кластера, с которого надо начать читать с учетом смещения
    uint32_t first_cluster = offset / cluster_sz;
    // Смещение внутри первого кластера, с которого надо начать читать
    uint32_t cluster_offset = offset % cluster_sz;
    // Порядковый номер текущего кластера
    uint32_t current_cluster_num = 0;

    uint32_t new_cluster_id;
    int cluster_was_allocated = 0;

    ssize_t written = 0;

    // Сразу проверим, не пустой ли файл
    if (current_cluster == 0)
    {
        // Файл не имел ни одного кластера
        // Не проблема - выделяем кластер
        if ((rc = fat_alloc_cluster(inst, &new_cluster_id)) != 0) {
            written = rc;
            goto exit;
        }

        // Запишем выделенный кластер первым для файла
        direntry.first_cluster_lo = (new_cluster_id & 0xFFFF);
        direntry.first_cluster_hi = (new_cluster_id >> 16) & 0xFFFF;

        cluster_was_allocated = TRUE;
        current_cluster = new_cluster_id;
    }

    while ((count - written) > 0)
    {
        // надо добраться до нужного кластера
        if (current_cluster_num >= first_cluster)
        {
            if (cluster_was_allocated == FALSE)
            {
                // для ускорения работы, считываем старые данные кластера, только если он уже существовал в файле
                if ((rc = fat_read_cluster(inst, current_cluster, cluster_buffer)) != 0) {
                    written = rc;
                    goto exit;   
                }
            }
            
            ssize_t available_data = MIN(count - written, cluster_sz - cluster_offset);
            
            // Копирование в буфер кластера новых данных для записи
            memcpy(cluster_buffer + cluster_offset, buffer + written, available_data);
            cluster_offset = 0;

            //printk("Writing to cluster %i\n", current_cluster);
            // запишем новые данные кластера
            if ((rc = fat_write_cluster(inst, current_cluster, cluster_buffer)) != 0) {
                written = rc;
                goto exit;   
            }

            written += available_data;
        }

        if ((count - written) <= 0)
        {
            break;
        }

        // Заранее снимаем флаг того, что последний кластер - новый
        cluster_was_allocated = FALSE;

        // Получить номер следующего кластера
        rc = fat_get_next_cluster(inst, current_cluster, &next_cluster);
        if (rc != 0)
        {
            written = rc;
            goto exit;
        }

        //printk("Next cluster %i (%p)\n", next_cluster, next_cluster);

        // следующего кластера еще нет
        if (fat_is_eoc(inst, next_cluster))
        {
            //printk("Alloc cluster\n");
            // выделить кластер
            rc = fat_alloc_cluster(inst, &new_cluster_id);
            if (rc != 0) {
                written = rc;
                goto exit;
            }

            //printk("Allocated cluster %i\n", new_cluster_id);

            // и добавить к файлу
            rc = fat_set_next_cluster(inst, current_cluster, new_cluster_id);
            if (rc != 0) {
                written = rc;
                goto exit;
            }

            cluster_was_allocated = TRUE;
            // Следующий кластер - новый кластер
            next_cluster = new_cluster_id;
        }

        // Переходим на следующий кластер
        current_cluster = next_cluster;

        // Увеличиваем порядковый номер кластера внутри файла
        current_cluster_num ++;
    }

    // обновить размер, если необходимо
    if (direntry.file_size < offset + count)
        direntry.file_size = offset + count;

    // добавить флаг "готов к архивированию", так как мы изменили файл
    direntry.attr |= FILE_ATTR_ARCHIVE;

    // Перезаписать на диск
    fat_write_dentry(inst, vfs_inode->inode, &direntry);

    // обновить размер vfs inode
    vfs_inode->size = direntry.file_size;
    //inode->blocks = e2_inode.num_blocks;

    // Сдвинуть
    file->pos += written;

exit:
    kfree(cluster_buffer);
    return written;
}

int fat_truncate(struct inode* inode, size_t len)
{
    struct fat_instance* inst = inode->sb->fs_info;

    struct fat_direntry direntry;
    int rc = fat_read_dentry(inst, inode->inode, &direntry);
    if (rc != 0)
    {
        return rc;
    }

    uint32_t old_size = direntry.file_size;
    // установка размера
    direntry.file_size = (uint32_t) len;
    // добавить флаг "готов к архивированию", так как мы изменили файл
    direntry.attr |= FILE_ATTR_ARCHIVE;

    // старое количество кластеров
    uint32_t old_clusters_count = align(old_size, inst->bytes_per_cluster) / inst->bytes_per_cluster;
    // новое количество кластеров
    uint32_t new_clusters_count = (len == 0) ? 0 : align(len, inst->bytes_per_cluster) / inst->bytes_per_cluster;
    // номер первого кластера
    uint32_t current_cluster = fat_get_first_cluster_idx(inst, &direntry);
    uint32_t next_cluster;
    uint32_t current_cluster_pos = 0;

    //printk("new len %i old %i. new clusters %i old %i\n", len, old_size, new_clusters_count, old_clusters_count);

    if (len == 0)
    {
        direntry.first_cluster_lo = 0;
        direntry.first_cluster_hi = 0;
    }

    if (new_clusters_count < old_clusters_count)
    {
        // Если количество кластеров для нового размера меньше, чем было
        // Освобождаем выделенные файлу блоки
        while (current_cluster != 0 && fat_is_eoc(inst, current_cluster)) 
        {
            // получаем номер следующего кластера
            if ((rc = fat_get_next_cluster(inst, current_cluster, &next_cluster)) != 0)
                return rc;

            // освобождаем блоки, сверх того размера, который нужно оставить
            if (current_cluster_pos >= new_clusters_count)
            {
                //printk("freeing cluster %i at pos %i\n", current_cluster, current_cluster_pos);
                if ((rc = fat_free_cluster(inst, current_cluster)) != 0)
                    return rc;
            }

            current_cluster = next_cluster;
            current_cluster_pos ++;
        }
    }

    rc = fat_write_dentry(inst, inode->inode, &direntry);
    if (rc != 0)
    {
        return rc;
    }

    // обновить размер inode
    inode->size = len;

    return 0;
}

int fat_add_direntry(struct fat_instance* inst, int is_root, struct fat_direntry *parent, struct fat_direntry *new_direntry, uint16_t *lfn_buffer, size_t lfn_len) 
{   
    int rc = -ENOSPC;
    // буфер под кластер
    char *cluster_buffer = NULL;

    // Буфер под кластер
    uint32_t cluster_sz = fat_get_directory_buffer_size(inst, is_root);
    // буфер под кластер
    cluster_buffer = kmalloc(cluster_sz);
    if (cluster_buffer == NULL) {
        rc = -ENOMEM;
        goto exit;
    }

    uint32_t current_cluster_idx = 0;
    uint32_t current_cluster_id = 0;
    uint64_t base_sector = 0;
    struct fat_direntry*    d_ptr;
    struct fat_direntry*    first_free;

    uint16_t lfn_part[13];
    // рассчитать количество необходимых данных для хранения LFN
    size_t required_lfn_len = ((lfn_len + 13 - 1) / 13);
    
    while (1)
    {
        rc = fat_read_directory_cluster(inst, parent, is_root, current_cluster_idx ++, cluster_buffer, &base_sector, &current_cluster_id);

        if (rc == FAT_DIR_LAST_CLUSTER) {            
            // Места в папке нет
            if (is_root && (inst->fs_type == FS_FAT16 || inst->fs_type == FS_FAT12)) {
                // если это FAT12 или FAT16, то корневая директория состоит из определенного 
                // количества секторов и не является расширяемой
                rc = -ENOSPC;
                goto exit;
            }

            // выделяем новый кластер
            uint32_t new_cluster_id;
            if ((rc = fat_alloc_cluster(inst, &new_cluster_id)) != 0)
                goto exit;

            // добавляем выделенный кластер к папке
            if ((rc = fat_set_next_cluster(inst, current_cluster_id, new_cluster_id)) != 0)
                goto exit;

            // зануляем буфер
            memset(cluster_buffer, 0, cluster_sz);

            // новый кластер делаем текущим
            current_cluster_id = new_cluster_id;
        } else if (rc != 0) {
            goto exit;
        }

        uint32_t pos = 0;
        size_t free_records = 0;

        while (pos < cluster_sz) 
        {
            d_ptr = (struct fat_direntry*) &cluster_buffer[pos];

            // Проверяем, свободен ли direntry
            if (d_ptr->name[0] == 0xE5 || d_ptr->name[0] == 0x00)
                free_records ++;
            else 
                free_records = 0;

            // нашли достаточно свободных блоков по 32 байта
            if (free_records == (required_lfn_len + 1)) 
            {
                // в самый последний блок (d_ptr) запишем SFN
                memcpy(d_ptr, new_direntry, sizeof(struct fat_direntry));

                // первый блок, в который пишем LFN
                first_free = d_ptr - free_records + 1;

                for (size_t lfn_i = 0; lfn_i < required_lfn_len; lfn_i ++) 
                {
                    struct fat_lfn *lfn_ptr = (struct fat_lfn*) &first_free[lfn_i];
                    lfn_ptr->attr = FILE_ATTR_LFN;
                    // номера в обратном порядке
                    lfn_ptr->order = (required_lfn_len - lfn_i);
                    // для первого LFN ставим 0x40
                    if (lfn_i == 0) 
                        lfn_ptr->order |= 0x40;
                    // считаем checksum из SFN
                    lfn_ptr->checksum = fat_get_lfn_checksum(new_direntry->name);
                    // тут должны быть нули
                    lfn_ptr->type = 0x0;
                    lfn_ptr->zero = 0x0000;

                    // заполним LFN часть 0xFF
                    memset(lfn_part, 0xFF, 13 * sizeof(uint16_t));
                    // LFN располагаются в обратном порядке
                    size_t namepart_offset = (required_lfn_len - lfn_i - 1) * 13;
                    uint16_t *name_offset = &lfn_buffer[namepart_offset];
                    // размер строки в текущем LFN блоке
                    size_t namepart_len = MIN(lfn_len - namepart_offset, 13);
                    // копируем часть строки
                    memcpy(lfn_part, name_offset, sizeof(uint16_t) * namepart_len);
                    // терминируем строку
                    if (namepart_len < 13) 
                        lfn_part[namepart_len] = 0x0;

                    //printk("namepart_offset %i namepart_len %i\n", namepart_offset, namepart_len);

                    // копируем UTF-16 строку по частям
                    memcpy(lfn_ptr->name_0, lfn_part, sizeof(lfn_ptr->name_0));   
                    memcpy(lfn_ptr->name_1, &lfn_part[5], sizeof(lfn_ptr->name_1));   
                    memcpy(lfn_ptr->name_2, &lfn_part[11], sizeof(lfn_ptr->name_2));   
                }

                // перезапишем измененный кластер
                rc = fat_write_directory_cluster(inst, is_root, current_cluster_id, cluster_buffer);
                goto exit;
            }

            // Увеличить смещение
            pos += sizeof(struct fat_direntry);
        }
    }

exit:
    KFREE_SAFE(cluster_buffer)
    return rc;
}

int fat_mkfile(struct inode* parent, const char* file_name, uint32_t mode)
{
    (void) mode;

    int rc;
    struct fat_instance* inst = parent->sb->fs_info;

    size_t name_len;
    int is_root_directory = parent == inst->vfs_root_inode;

    // родительская директория
    struct fat_direntry parent_direntry;
    if (is_root_directory == FALSE) {
        if ((rc = fat_read_dentry(inst, parent->inode, &parent_direntry)) != 0) {
            return rc;
        }
    }

    // получим текущее время
    struct timespec current_time;
    get_time_epoch(&current_time);

    // заполним создаваемую direntry
    struct fat_direntry new_direntry;
    // по умолчанию все созданные файлы должны быть готовы для архивирования
    // в данном случае имеется в виду именно резервное копирование, а не сжатие
    new_direntry.attr = FILE_ATTR_ARCHIVE;
    new_direntry.nt_reserved = 0;
    new_direntry.file_size = 0;
    new_direntry.first_cluster_hi = 0;
    new_direntry.first_cluster_lo = 0;
    // заполняем время
    fat_epoch_to_datetime(current_time.tv_sec, &new_direntry.creat_date, &new_direntry.creat_time);
    fat_epoch_to_datetime(current_time.tv_sec, &new_direntry.write_date, &new_direntry.write_time);
    fat_epoch_to_datetime(current_time.tv_sec, &new_direntry.last_access_date, NULL);
    // предварительно заполняем SFN пробелами
    memset(new_direntry.name, ' ', sizeof(new_direntry.name));

    // Генерация SFN
    char *file_name_dot_ptr = strchr(file_name, '.');
    if (file_name_dot_ptr) 
    {
        name_len = file_name_dot_ptr - file_name;
        // длина расширения
        size_t ext_len = file_name + strlen(file_name) - file_name_dot_ptr - 1;
        // обрезаем расширение до 3 знаков
        ext_len = MIN(ext_len, 3);
        // копируем расширение
        memcpy(&new_direntry.name[8], file_name_dot_ptr + 1, ext_len);
    }
    else 
    {
        name_len = strlen(file_name);
    }

    // копируем имя
    memcpy(new_direntry.name, file_name, MIN(name_len, 8));
    if (name_len > 8) 
    {
        new_direntry.name[6] = '~';
        // TODO: проверять, нет ли уже таких SFN в этой папке
        new_direntry.name[7] = '1';
    }

    // винда очень хочет, чтобы SFN содержала только большие буквы
    fat_toupper(new_direntry.name, 11);

    // TODO: нормальная конвертация в UTF-16
    uint16_t *utf16_buffer = kmalloc(sizeof(uint16_t) * FAT_LFN_MAX_SZ);
    for (size_t i = 0; i < strlen(file_name); i ++) {
        utf16_buffer[i] = file_name[i];
    }

    rc = fat_add_direntry(inst, is_root_directory, &parent_direntry, &new_direntry, utf16_buffer, strlen(file_name));

    kfree(utf16_buffer);

    return rc;
}


int fat_mkdir(struct inode* parent, const char* dir_name, uint32_t mode)
{
    (void) mode;

    int rc;
    struct fat_instance* inst = parent->sb->fs_info;

    size_t name_len;
    int is_root_directory = parent == inst->vfs_root_inode;

    uint16_t *lfn_buffer = NULL;
    void *cluster_buffer = NULL;

    // родительская директория
    struct fat_direntry parent_direntry;
    if (is_root_directory == FALSE) {
        if ((rc = fat_read_dentry(inst, parent->inode, &parent_direntry)) != 0) {
            return rc;
        }
    }

    // проверить, нет ли уже такого имени в директории
    if (fat_find_dentry(parent->sb, parent, dir_name, NULL) != WRONG_INODE_INDEX) {
        return -ERROR_ALREADY_EXISTS;
    }

    // получим текущее время
    struct timespec current_time;
    get_time_epoch(&current_time);

    // заполним создаваемую direntry
    struct fat_direntry new_direntry;
    // по умолчанию все созданные файлы должны быть готовы для архивирования
    // в данном случае имеется в виду именно резервное копирование, а не сжатие
    new_direntry.attr = FILE_ATTR_DIRECTORY | FILE_ATTR_ARCHIVE;
    new_direntry.nt_reserved = 0;
    new_direntry.file_size = 0;
    // заполняем время
    fat_epoch_to_datetime(current_time.tv_sec, &new_direntry.creat_date, &new_direntry.creat_time);
    fat_epoch_to_datetime(current_time.tv_sec, &new_direntry.write_date, &new_direntry.write_time);
    fat_epoch_to_datetime(current_time.tv_sec, &new_direntry.last_access_date, NULL);
    // предварительно заполняем SFN пробелами
    memset(new_direntry.name, ' ', sizeof(new_direntry.name));

    // Генерация SFN
    char *file_name_dot_ptr = strchr(dir_name, '.');
    if (file_name_dot_ptr) 
    {
        name_len = file_name_dot_ptr - dir_name;
        // длина расширения
        size_t ext_len = dir_name + strlen(dir_name) - file_name_dot_ptr - 1;
        // обрезаем расширение до 3 знаков
        ext_len = MIN(ext_len, 3);
        // копируем расширение
        memcpy(&new_direntry.name[8], file_name_dot_ptr + 1, ext_len);
    }
    else 
    {
        name_len = strlen(dir_name);
    }

    // копируем имя
    memcpy(new_direntry.name, dir_name, MIN(name_len, 8));
    if (name_len > 8) 
    {
        new_direntry.name[6] = '~';
        // TODO: проверять, нет ли уже таких SFN в этой папке
        new_direntry.name[7] = '1';
    }

    // винда очень хочет, чтобы SFN содержала только большие буквы
    fat_toupper(new_direntry.name, 11);

    // TODO: нормальная конвертация в UTF-16
    lfn_buffer = kmalloc(sizeof(uint16_t) * FAT_LFN_MAX_SZ);
    for (size_t i = 0; i < strlen(dir_name); i ++) {
        lfn_buffer[i] = dir_name[i];
    }

    // заранее выделим память под кластер перед любыми изменениями на диске
    size_t cluster_buffer_sz = fat_get_directory_buffer_size(inst, FALSE);
    cluster_buffer = kmalloc(cluster_buffer_sz);
    if (cluster_buffer == NULL) {
        rc = -ENOMEM;
        goto exit;
    }

    // занулим выделенную память, чтобы на диск ни в коем случае не попал ни один секретик
    memset(cluster_buffer, 0, cluster_buffer_sz);
    struct fat_direntry *cluster_dirents = (struct fat_direntry*) cluster_buffer;

    // 1. выделяем первый кластер для создаваемой директории
    uint32_t directory_cluster;
    if ((rc = fat_alloc_cluster(inst, &directory_cluster)) != 0)
        goto exit;

    // записать первый кластер в создаваемую direntry
    new_direntry.first_cluster_hi = (directory_cluster >> 16) & 0xFFFF;
    new_direntry.first_cluster_lo = (directory_cluster & 0xFFFF);

    // заполним direntry для этой же директории
    memset(cluster_dirents[0].name, ' ', 11);
    cluster_dirents[0].name[0] = '.';
    cluster_dirents[0].attr = FILE_ATTR_DIRECTORY | FILE_ATTR_ARCHIVE;
    cluster_dirents[0].nt_reserved = 0;
    cluster_dirents[0].file_size = 0;
    cluster_dirents[0].first_cluster_hi = (directory_cluster >> 16) & 0xFFFF;
    cluster_dirents[0].first_cluster_lo = (directory_cluster & 0xFFFF);

    // заполним direntry для директории (..), в которой создаем новую директорию
    memset(cluster_dirents[1].name, ' ', 11);
    cluster_dirents[1].name[0] = '.';
    cluster_dirents[1].name[1] = '.';
    cluster_dirents[1].attr = FILE_ATTR_DIRECTORY | FILE_ATTR_ARCHIVE;
    cluster_dirents[1].nt_reserved = 0;
    cluster_dirents[1].file_size = 0;
    if (is_root_directory == TRUE) 
    {
        // для FAT12, FAT16 корневая директория состоит из секторов, а не из кластеров
        if (inst->fs_type == FS_FAT12 || inst->fs_type == FS_FAT16) {
            cluster_dirents[1].first_cluster_lo = 0;
            cluster_dirents[1].first_cluster_hi = 0;
        } else if (inst->fs_type == FS_FAT32) {
            cluster_dirents[1].first_cluster_lo = (inst->bpb->ext_32.root_cluster & 0xFFFF);
            cluster_dirents[1].first_cluster_hi = (inst->bpb->ext_32.root_cluster >> 16) & 0xFFFF;
        }
    }
    else 
    {
        cluster_dirents[1].first_cluster_lo = parent_direntry.first_cluster_lo;
        cluster_dirents[1].first_cluster_hi = parent_direntry.first_cluster_hi;
    }

    // 2. записываем первый кластер
    if ((rc = fat_write_cluster(inst, directory_cluster, cluster_buffer)) != 0)
        goto exit;

    // 3. добавляем direntry в родительскую директорию
    if ((rc = fat_add_direntry(inst, is_root_directory, &parent_direntry, &new_direntry, lfn_buffer, strlen(dir_name))) != 0)
        goto exit;

exit:
    KFREE_SAFE(lfn_buffer);
    KFREE_SAFE(cluster_buffer);
    return rc;
}

int fat_statfs(struct superblock *sb, struct statfs* stat)
{
    struct fat_instance* inst = (struct fat_instance*) sb->fs_info;

    stat->fstype = 0x4d44;
    stat->blocksize = inst->bpb->bytes_per_sector;

    stat->blocks = inst->sectors_count;
    stat->blocks_free = inst->free_clusters * inst->bpb->sectors_per_cluster;

    return 0;
}

int fat_unmount(struct superblock* sb)
{
    struct fat_instance* inst = (struct fat_instance*) sb->fs_info;
    fat_free_instance(inst);
    return 0;
}