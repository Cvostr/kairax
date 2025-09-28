#include "../vfs/filesystems.h"
#include "fat.h"
#include "stdio.h"
#include "mem/kheap.h"
#include "kairax/stdio.h"
#include "string.h"
#include "kairax/kstdlib.h"

struct super_operations fat_sb_ops;

// ---- file operations
struct file_operations fat_dir_ops;
struct file_operations fat_file_ops;

// ---- inode operations
struct inode_operations fat_file_inode_ops;
struct inode_operations fat_dir_inode_ops;

#define FAT_MOUNT_LOG

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

    memset(&fat_dir_ops, 0, sizeof(struct file_operations));
    fat_dir_ops.readdir = fat_file_readdir;

    memset(&fat_file_ops, 0, sizeof(struct file_operations));
    fat_file_ops.read = fat_file_read;
}

int fat_read_sector(struct fat_instance* inst, uint64_t first_sector, uint64_t sectors, char* buffer)
{
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

int fat_read_cluster(struct fat_instance* inst, uint32_t cluster, char* buffer)
{
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
    if (partition_read(drive, 0, 1, instance->bpb) != 0)
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

    instance->first_fat_sector = bpb->reserved_sector_count;
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
        instance->eoc_value = FAT12_EOC;
    } else if(instance->total_clusters < 65525) {
        instance->fs_type = FS_FAT16;
        // Размер для буфера FAT
        instance->fat_buffer_size = instance->bytes_per_sector;
        // значение EOC
        instance->eoc_value = FAT16_EOC;
    } else {
        instance->fs_type = FS_FAT32;
        // Размер для буфера FAT
        instance->fat_buffer_size = instance->bytes_per_sector;
        // значение EOC
        instance->eoc_value = FAT32_EOC;
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

    // Вычислить свободные кластера
    instance->free_clusters = fat_calc_free_clusters(instance);

#ifdef FAT_MOUNT_LOG
    printk("FAT: type %i FATs %i FAT size %i FirstFat %i sectors %i BPS %i Clusters %i Free %i SPC %i BPC %i FatBufferSz %i\n", 
        instance->fs_type, 
        bpb->fats_count,
        instance->fat_size,
        instance->first_fat_sector,
        instance->sectors_count,
        bpb->bytes_per_sector,
        instance->total_clusters,
        instance->free_clusters,
        bpb->sectors_per_cluster,
        instance->bytes_per_cluster,
        instance->fat_buffer_size);
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

/*
    struct fat_direntry root_direntry;
    fat_read_dentry(instance, ino_num, &root_direntry);
    printk("FAT: Root dentry name %s sector %i sectors %i\n", root_direntry.name, instance->root_dir_sector, instance->root_dir_sectors);
*/
    struct inode* result = new_vfs_inode();
    instance->vfs_root_inode = result;
    result->inode = ino_num;
    result->sb = sb;
    result->uid = 0;
    result->gid = 0;
    result->size = 0;
    result->mode = INODE_TYPE_DIRECTORY | 0777;
    result->access_time = 0;
    result->create_time = 0;
    result->modify_time = 0;
    result->hard_links = 1;
    result->device = (dev_t) instance;
    result->file_ops = &fat_dir_ops;
    result->operations = &fat_dir_inode_ops;

    sb->fs_info = instance;
    sb->blocksize = bpb->bytes_per_sector;
    sb->operations = &fat_sb_ops;

    return result;
}

uint32_t fat_calc_free_clusters(struct fat_instance* inst)
{
    uint32_t last_sector = 0;
    uint32_t counter = 0;
    char* fat_buffer = kmalloc(inst->fat_buffer_size);

    for (uint32_t i = 0; i < inst->total_clusters; i ++)
    {
        uint32_t next = fat_get_next_cluster_ex(inst, i, fat_buffer, &last_sector);
        if (next == 0) counter ++;
    }

    kfree(fat_buffer);
    return counter;
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

uint32_t fat_get_next_cluster(struct fat_instance* inst, uint32_t cluster)
{
    uint32_t sector;
    // Выделить буфер под FAT
    char* fat_buffer = kmalloc(inst->fat_buffer_size);
    uint32_t result = fat_get_next_cluster_ex(inst, cluster, fat_buffer, &sector);
    KFREE_SAFE(fat_buffer);

    return result;
}

uint32_t fat_get_next_cluster_ex(struct fat_instance* inst, uint32_t cluster, char* fat_buffer, uint32_t* last_sector)
{
    uint32_t result = 0;
    uint32_t fat_offset = 0;    // Смещение внутри таблицы FAT в байтах
    uint32_t fat_sector = 0;    // Номер сектора в таблице FAT (Смещение внутри таблицы FAT в секторах)
    uint32_t sector_offset = 0; // Смещение внутри сектора таблицы FAT

    switch (inst->fs_type)
    {
        case FS_FAT12:
            fat_offset = cluster + (cluster / 2);  
            fat_sector = inst->first_fat_sector + (fat_offset / inst->bytes_per_sector); 
            sector_offset = fat_offset % inst->bytes_per_sector;
            
            if (*last_sector != fat_sector)
                fat_read_sector(inst, fat_sector, 2, fat_buffer);
            
            result = *((uint16_t*) &fat_buffer[sector_offset]);
            result = (cluster & 1) ? result >> 4 : result & 0xfff;
            break;
        case FS_FAT16:
            fat_offset = cluster * sizeof(uint16_t);
            fat_sector = inst->first_fat_sector + (fat_offset / inst->bytes_per_sector);
            sector_offset = fat_offset % inst->bytes_per_sector;

            if (*last_sector != fat_sector)
                fat_read_sector(inst, fat_sector, 1, fat_buffer);

            result = *((uint16_t*) &fat_buffer[sector_offset]);
            break;
        case FS_FAT32:
        case FS_EXFAT:
            fat_offset = cluster * sizeof(uint32_t);
            fat_sector = inst->first_fat_sector + (fat_offset / inst->bytes_per_sector);
            sector_offset = fat_offset % inst->bytes_per_sector;

            if (*last_sector != fat_sector)
                fat_read_sector(inst, fat_sector, 1, fat_buffer);

            result = result = *((uint32_t*) &fat_buffer[sector_offset]);
            if (inst->fs_type == FS_FAT32)
            {
                result = result &= 0x0FFFFFFF;
            }
            break;
    }

    *last_sector = fat_sector;

    return result;
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
    result->access_time = fat_date_to_epoch(&direntry.last_access_date, NULL);
    result->create_time = fat_date_to_epoch(&direntry.creat_date, &direntry.creat_time);
    result->modify_time = fat_date_to_epoch(&direntry.write_date, &direntry.write_time);
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
                                uint64_t* first_sector)
{
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
                int rc = fat_read_sector(inst, current_cluster, 1, buffer);
                if (rc != 0)
                {
                    return rc;
                }

                *first_sector = current_cluster;

                return 0;
            case FS_FAT32:
                // Для Fat32 кластер корневой директории задан в заголовке
                current_cluster = inst->bpb->ext_32.root_cluster;
        }
    } else {
        // Если это не корневая папка - берем первый кластер из dentry
        current_cluster = fat_get_first_cluster_idx(inst, direntry);
    }

    // Вычислим номер нужного кластера
    for (uint32_t i = 0; i < index; i ++)
    {
        current_cluster = fat_get_next_cluster(inst, current_cluster);
    }

    uint32_t EOC = inst->eoc_value;
    if (current_cluster == EOC)
    {
        return FAT_DIR_LAST_CLUSTER;
    }

    // Переводим кластер в сектор
    uint32_t    spc = inst->bpb->sectors_per_cluster;
    *first_sector = ((current_cluster - 2) * spc) + inst->first_data_sector;

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

    if (order < 1 || order > 20)
        return;

    uint32_t lfn_pos = (order - 1) * 13;

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
    struct fat_instance* inst = sb->fs_info;

    char* checking_name = NULL;
    char* cluster_buffer = NULL;

    int is_root_directory = parent_inode == inst->vfs_root_inode;
    uint64_t result_inode = WRONG_INODE_INDEX;

    //printk("fat_find_dentry (ino %i (root = %i) name %s)\n", parent_inode->inode, is_root_directory, name);

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
    // буфер под кластер
    cluster_buffer = kmalloc(cluster_sz);
    if (cluster_buffer == NULL) {
        result_inode = WRONG_INODE_INDEX;
        goto exit;
    }

    uint32_t current_cluster_idx = 0;
    uint64_t base_sector = 0;

    checking_name[0] = '\0';

    while (fat_read_directory_cluster(inst, &dir_direntry, is_root_directory, current_cluster_idx ++, cluster_buffer, &base_sector) != FAT_DIR_LAST_CLUSTER)
    {
        uint32_t pos = 0;

        struct fat_direntry*    d_ptr;

        while (pos < cluster_sz) 
        {
            d_ptr = (struct fat_direntry*) &cluster_buffer[pos];

            if (d_ptr->attr == FILE_ATTR_LFN) {
                // Встретилась запись LFN
                fat_apply_lfn(checking_name, (struct fat_lfn*) d_ptr);
                pos += sizeof(struct fat_lfn);
            } else {

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
    KFREE_SAFE(cluster_buffer)
    return result_inode;
}

struct dirent* fat_file_readdir(struct file* dir, uint32_t index)
{
    struct inode* vfs_inode = dir->inode;
    struct superblock* sb = vfs_inode->sb;
    struct fat_instance* inst = sb->fs_info;

    struct dirent* result = NULL;
    char* cluster_buffer = NULL;
    char* checking_name = NULL;
    int is_root_directory = vfs_inode == inst->vfs_root_inode;
    
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
    checking_name[0] = '\0';

    uint32_t current_cluster_idx = 0;
    uint64_t base_sector = 0;
    uint32_t dentry_iter = 0;
    struct fat_direntry*    d_ptr;
    
    while (fat_read_directory_cluster(inst, &direntry, is_root_directory, current_cluster_idx ++, cluster_buffer, &base_sector) != FAT_DIR_LAST_CLUSTER)
    {
        uint32_t pos = 0;

        while (pos < cluster_sz) 
        {
            d_ptr = (struct fat_direntry*) &cluster_buffer[pos];

            if (d_ptr->attr == FILE_ATTR_LFN) 
            {
                // Встретилась запись LFN
                fat_apply_lfn(checking_name, (struct fat_lfn*) d_ptr);
                pos += sizeof(struct fat_lfn);
            } else {

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
                        strncpy(result->name, checking_name, FAT_LFN_MAX_SZ);
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
    uint32_t EOC = inst->eoc_value;
    ssize_t readed = 0;

    while (current_cluster < EOC && (count - readed) > 0)
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
        current_cluster = fat_get_next_cluster_ex(inst, current_cluster, fat_buffer, &last_sector);
        current_cluster_num ++;
    }

    // Сдвинуть
    file->pos += readed;

exit:
    kfree(fat_buffer);
    kfree(cluster_buffer);
    return readed;
}

int fat_statfs(struct superblock *sb, struct statfs* stat)
{
    struct fat_instance* inst = (struct fat_instance*) sb->fs_info;

    stat->blocksize = inst->bpb->bytes_per_sector;

    stat->blocks = inst->sectors_count;
    stat->blocks_free = inst->free_clusters * inst->bpb->sectors_per_cluster;

    return 0;
}

int fat_unmount(struct superblock* sb)
{

}