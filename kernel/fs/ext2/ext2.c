#include "ext2.h"
#include "../vfs/filesystems.h"
#include "string.h"
#include "memory/mem_layout.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "stdio.h"
#include "errors.h"
#include "kairax/time.h"
#include "kairax/kstdlib.h"

struct inode_operations file_inode_ops;
struct inode_operations dir_inode_ops;
struct inode_operations symlink_inode_ops;

struct file_operations file_ops;
struct file_operations dir_ops;

struct super_operations sb_ops;

void ext2_init()
{
    filesystem_t* ext2fs = new_filesystem();
    ext2fs->name = "ext2";
    ext2fs->mount = ext2_mount;
    ext2fs->unmount = ext2_unmount;

    filesystem_register(ext2fs);

    file_inode_ops.chmod = ext2_chmod;
    file_inode_ops.truncate = ext2_truncate;

    dir_inode_ops.mkdir = ext2_mkdir;
    dir_inode_ops.mkfile = ext2_mkfile;
    dir_inode_ops.chmod = ext2_chmod;
    dir_inode_ops.rename = ext2_rename;
    dir_inode_ops.unlink = ext2_unlink;
    dir_inode_ops.link = ext2_linkat;
    dir_inode_ops.rmdir = ext2_rmdir;
    dir_inode_ops.mknod = ext2_mknod;
    dir_inode_ops.symlink = ext2_symlink;
    dir_ops.readdir = ext2_file_readdir;

    // Symlink inode
    symlink_inode_ops.readlink = ext2_readlink;

    // File
    file_ops.read = ext2_file_read;
    file_ops.write = ext2_file_write;

    // Superblock
    sb_ops.destroy_inode = ext2_purge_inode;
    sb_ops.read_inode = ext2_read_node;
    sb_ops.find_dentry = ext2_find_dentry;
    sb_ops.stat = ext2_statfs;
}

ext2_inode_t* new_ext2_inode()
{
    ext2_inode_t* ext2_inode = kmalloc(sizeof(ext2_inode_t));
    memset(ext2_inode, 0, sizeof(ext2_inode_t));
    return ext2_inode;
}

uint32_t ext2_partition_read_block_virt(ext2_instance_t* inst, uint64_t block_start, uint64_t blocks, char* buffer)
{
    uint64_t    start_lba = block_start * (inst->block_size / 512);
    uint64_t    lba_count = blocks  * (inst->block_size / 512);
    return partition_read(inst->partition, start_lba, lba_count, buffer);
}

uint32_t ext2_partition_write_block_virt(ext2_instance_t* inst, uint64_t block_start, uint64_t blocks, char* buffer)
{
    uint64_t    start_lba = block_start * inst->block_size / 512;
    uint64_t    lba_count = blocks  * inst->block_size / 512;
    return partition_write(inst->partition, start_lba, lba_count, buffer);
}

uint32_t ext2_inode_block_absolute(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_block_index)
{
    uint32_t level = inst->block_size / 4;
    int d, e, f, g;
    if (inode_block_index < EXT2_DIRECT_BLOCKS) {
        return inode->blocks[inode_block_index];
    }

    int a = inode_block_index - EXT2_DIRECT_BLOCKS;
    //Выделение временной памяти под считываемые блоки
    uint32_t* tmp = kmalloc(inst->block_size);
    int b = a - level;
    if(b < 0) {
        ext2_partition_read_block_virt(inst, inode->blocks[EXT2_DIRECT_BLOCKS], 1, tmp);
        uint32_t result = tmp[a];
        kfree(tmp);
        return result;
    }
    int c = b - level * level;
    if(c < 0) {
        c = b / level;
        d = b - c * level;
        ext2_partition_read_block_virt(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1, tmp);
        ext2_partition_read_block_virt(inst, tmp[c], 1, tmp);
        uint32_t result = tmp[d];
        kfree(tmp);
        return result;
    }
    d = c - level * level * level;
    if(d < 0) {
        e = c / (level * level);
        f = (c - e * level * level) / level;
        g = (c - e * level * level - f * level);
        ext2_partition_read_block_virt(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 2], 1, tmp);
        ext2_partition_read_block_virt(inst, tmp[e], 1, tmp);
        ext2_partition_read_block_virt(inst, tmp[f], 1, tmp);
        uint32_t result = tmp[g];
        kfree(tmp);
        return result;
    }
    return 0;   
}

int ext2_inode_add_block(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_index, uint32_t abs_block, uint32_t inode_block)
{
    uint32_t level = inst->block_size / 4;
    int rc = -1;

    char* buffer = (char*)kmalloc(inst->block_size);

    if (inode_block < EXT2_DIRECT_BLOCKS) {

        inode->blocks[inode_block] = abs_block;

        rc = 1;
        goto exit;

    } else if (inode_block < EXT2_DIRECT_BLOCKS + level) {
        // Номер блока помещается в 1й дополнительный блок

        if (!inode->blocks[EXT2_DIRECT_BLOCKS]) {
            // Адрес доп. блока не задан - выделяем блок и задаем адрес
            uint32_t new_block_index = ext2_alloc_block(inst);
            
            if (new_block_index == 0) {
                printk("Failed to allocate block!");
                rc = -2;
                goto exit;
            }

            inode->blocks[EXT2_DIRECT_BLOCKS] = new_block_index;
            ext2_write_inode_metadata(inst, inode, inode_index);
        }

        // Считать блок по адресу
        ext2_partition_read_block_virt(inst, inode->blocks[EXT2_DIRECT_BLOCKS], 1, buffer);

        // Записать адрес блока в дополнительный блок
        ((uint32_t*)buffer)[inode_block - EXT2_DIRECT_BLOCKS] = abs_block;

        // Записать измененный блок
        ext2_partition_write_block_virt(inst, inode->blocks[EXT2_DIRECT_BLOCKS], 1, buffer);
        
        rc = 1;
        goto exit;

    } else if (inode_block < EXT2_DIRECT_BLOCKS + level + level * level) {
        // Номер блока помещается в 2й дополнительный блок
        uint32_t a = inode_block - EXT2_DIRECT_BLOCKS;
		uint32_t b = a - level;
		uint32_t c = b / level;
		uint32_t d = b - c * level;

        if (!inode->blocks[EXT2_DIRECT_BLOCKS + 1]) {
            // Адрес доп. блока не задан - выделяем блок и задаем адрес
            uint64_t new_block_index = ext2_alloc_block(inst);
            if (!new_block_index) {
                rc = -2;
                goto exit;
            }
            inode->blocks[EXT2_DIRECT_BLOCKS + 1] = new_block_index;
            ext2_write_inode_metadata(inst, inode, inode_index);
        }

        // Считать блок по адресу
        ext2_partition_read_block_virt(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1, buffer);

        if (!((uint32_t*)buffer)[c]) {
            // Адрес доп. блока не задан - выделяем блок и задаем адрес
            uint64_t new_block_index = ext2_alloc_block(inst);
            if (!new_block_index) {
                rc = -2;
                goto exit;
            }
            ((uint32_t*)buffer)[c] = new_block_index;
            ext2_partition_write_block_virt(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1, buffer);
        }

        uint32_t saved_block = ((uint32_t*)buffer)[c];
        // Считать блок по адресу
        ext2_partition_read_block_virt(inst, saved_block, 1, buffer);

        // Записать адрес блока в дополнительный блок
        ((uint32_t*)buffer)[d] = abs_block;

        // Записать измененный блок
        ext2_partition_write_block_virt(inst, saved_block, 1, buffer);
        
        rc = 1;
        goto exit;

    } else if (inode_block < EXT2_DIRECT_BLOCKS + level + level * level + level) {
        // Номер блока помещается в 3й дополнительный блок
        uint32_t a = inode_block - EXT2_DIRECT_BLOCKS;
		uint32_t b = a - level;
		uint32_t c = b - level * level;
		uint32_t d = c / (level * level);
		uint32_t e = c - d * level * level;
		uint32_t f = e / level;
		uint32_t g = e - f * level;

        if (!inode->blocks[EXT2_DIRECT_BLOCKS + 2]) {
            // Адрес доп. блока не задан - выделяем блок и задаем адрес
            uint64_t new_block_index = ext2_alloc_block(inst);
            if (!new_block_index) {
                rc = -2;
                goto exit;
            }
            inode->blocks[EXT2_DIRECT_BLOCKS + 2] = new_block_index;
            ext2_write_inode_metadata(inst, inode, inode_index);
        }

        // Считать блок по адресу
        ext2_partition_read_block_virt(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 2], 1, buffer);

        if (!((uint32_t*)buffer)[d]) {
            // Адрес доп. блока не задан - выделяем блок и задаем адрес
            uint64_t new_block_index = ext2_alloc_block(inst);
            if (!new_block_index) {
                rc = -2;
                goto exit;
            }
            ((uint32_t*)buffer)[d] = new_block_index;
            ext2_partition_write_block_virt(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1, buffer);
        }

        uint32_t saved_block = ((uint32_t*)buffer)[d];
        // Считать блок по адресу
        ext2_partition_read_block_virt(inst, saved_block, 1, buffer);

        if (!((uint32_t*)buffer)[f]) {
            // Адрес доп. блока не задан - выделяем блок и задаем адрес
            uint64_t new_block_index = ext2_alloc_block(inst);
            if (!new_block_index) {
                rc = -2;
                goto exit;
            }
            ((uint32_t*)buffer)[f] = new_block_index;
            ext2_partition_write_block_virt(inst, saved_block, 1, buffer);
        }

        saved_block = ((uint32_t*)buffer)[f];
        // Считать блок по адресу
        ext2_partition_read_block_virt(inst, ((uint32_t*)buffer)[f], 1, buffer);

        // Записать адрес блока в дополнительный блок
        ((uint32_t*)buffer)[g] = abs_block;

        // Записать измененный блок
        ext2_partition_write_block_virt(inst, saved_block, 1, buffer);

        rc = 1;
        goto exit;
    }

exit:

    kfree(buffer);

    return rc;
}

uint32_t ext2_read_inode_block_virt(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_block, char* buffer)
{
    //Получить номер блока иноды внутри всего раздела
    uint32_t inode_block_abs = ext2_inode_block_absolute(inst, inode, inode_block);
    return ext2_partition_read_block_virt(inst, inode_block_abs, 1, buffer);
}

uint32_t ext2_write_inode_block_virt(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_num, uint32_t inode_block, char* buffer)
{
    uint32_t blksize512 = inst->block_size / 512;
    // Если пытаемся записать в невыделенный номер блока
    // Расширить размер inode
    while (inode_block >= inode->num_blocks / blksize512) {
        // Выделить блок
		ext2_alloc_inode_block(inst, inode, inode_num, inode->num_blocks / blksize512);
        // Обновить данные inode
		ext2_inode(inst, inode, inode_num);
	}

    //Получить номер блока иноды внутри всего раздела
    uint32_t inode_block_abs = ext2_inode_block_absolute(inst, inode, inode_block);
    return ext2_partition_write_block_virt(inst, inode_block_abs, 1, buffer);
}

int ext2_unmount(struct superblock* sb)
{
    ext2_instance_t* inst = (ext2_instance_t*)sb->fs_info;
    // Перезаписать на диск BGD и суперблок
    ext2_write_bgds(inst);
    ext2_rewrite_superblock(inst);
}

struct inode* ext2_mount(drive_partition_t* drive, struct superblock* sb)
{
    ext2_instance_t* instance = kmalloc(sizeof(ext2_instance_t));
    memset(instance, 0, sizeof(ext2_instance_t));

    instance->vfs_sb = sb;

    instance->partition = drive;
    instance->superblock = (ext2_superblock_t*)kmalloc(sizeof(ext2_superblock_t));
    instance->block_size = EXT2_SUPERBLOCK_SIZE;
    // Считать суперблок
    ext2_partition_read_block_virt(instance, 1, 1, instance->superblock);
    
    // Проверить магическую константу ext2
    if (instance->superblock->ext2_magic != EXT2_MAGIC) {
        kfree(instance->superblock);
        kfree(instance);
        return NULL;
    }

    if (instance->superblock->major >= 1) {
        // Ext2 версии 1 и новее, значит есть дополнительная часть superblock
        instance->file_size_64bit_flag = (instance->superblock->readonly_feature & EXT2_FEATURE_64BIT_FILE_SIZE) > 0;
        //printk("FEATURE %i\n", instance->superblock->required_feature & EXT2_FEATURE_DENTRY_FILE_TYPE);
    }

    // Сохранить некоторые полезные значения
    instance->block_size = (1024 << instance->superblock->log2block_size);
    instance->total_groups = instance->superblock->total_blocks / instance->superblock->blocks_per_group;
    while(instance->superblock->blocks_per_group * instance->total_groups < instance->superblock->total_blocks)
        instance->total_groups++;
    //необходимое количество блоков дескрипторов групп (BGD)
    instance->bgds_blocks = (instance->total_groups * sizeof(ext2_bgd_t)) / instance->block_size;
    while(instance->bgds_blocks * instance->block_size < instance->total_groups * sizeof(ext2_bgd_t))
        instance->bgds_blocks++;

    instance->bgds = (ext2_bgd_t*)kmalloc(instance->bgds_blocks * instance->block_size);
    // Если реальный размер блока - 1024, то структуры BGD идут сразу за суперблоком во втором блоке
    // Если больше - то номер блока BGD - 1
    instance->bgd_start_block = (instance->block_size == 1024) ? 2 : 1;
    //Чтение BGD
    ext2_partition_read_block_virt(instance, 
        instance->bgd_start_block,
        instance->bgds_blocks, 
        instance->bgds);
    //Чтение корневой иноды с индексом 2
    ext2_inode_t* ext2_inode_root = new_ext2_inode();
    ext2_inode(instance, ext2_inode_root, 2);
    //Создать объект VFS корневой иноды файловой системы 
    struct inode* result = ext2_inode_to_vfs_inode(instance, ext2_inode_root, 2);

    kfree(ext2_inode_root);

    //printk("Block size %i\n", instance->block_size);

    // Данные суперблока
    sb->fs_info = instance;
    sb->blocksize = instance->block_size;
    sb->operations = &sb_ops;

    return result;
}

void ext2_write_bgds(ext2_instance_t* inst)
{
    ext2_partition_write_block_virt( inst, 
                                inst->bgd_start_block,
                                inst->bgds_blocks,
                                inst->bgds);
}

void ext2_rewrite_superblock(ext2_instance_t* inst)
{
    uint64_t    start_lba = 1024 / 512;
    uint64_t    lba_count = EXT2_SUPERBLOCK_SIZE / 512;
    partition_write(inst->partition, start_lba, lba_count, inst->superblock);
}

uint32_t ext2_alloc_inode(ext2_instance_t* inst)
{
    uint32_t result = 0;
    uint32_t* bitmap_buffer = (uint32_t*)kmalloc(inst->block_size);
    // Найти свободный номер inode в битовой карте
    for (uint32_t bgd_i = 0; bgd_i < inst->total_groups; bgd_i++) {

        if (!inst->bgds[bgd_i].free_inodes)
            continue;

        // номер блока битмапы, в котором есть свободный номер inode
        uint32_t bitmap_block_index = inst->bgds[bgd_i].inode_bitmap;
        // считываем этот блок
        ext2_partition_read_block_virt(inst, bitmap_block_index, 1, bitmap_buffer);

        for (uint32_t j = 0; j < inst->block_size / 4; j ++) {
            uint32_t bitmap = bitmap_buffer[j];

            for (int bit_i = 0; bit_i < 32; bit_i ++) {
                int is_bit_free = !((bitmap >> bit_i) & 1);

                if (is_bit_free) {
                    // Найден номер свободной inode
                    bitmap_buffer[j] |= (1U << bit_i);
                    // Запишем обновленную битмапу с занятым битом
                    ext2_partition_write_block_virt(inst, bitmap_block_index, 1, bitmap_buffer);
                    // Уменьшим количество свободных inode в выбранном BGD
                    inst->bgds[bgd_i].free_inodes --;
                    // Запишем изменения на диск
                    ext2_write_bgds(inst);
                    // Уменьшить количество свободных inode в суперблоке и перезаписать его на диск
                    inst->superblock->free_inodes--;
                    ext2_rewrite_superblock(inst);

                    result = bgd_i * inst->superblock->inodes_per_group + j * 32 + bit_i + 1;

                    goto exit;
                }
            }
        }
    }

exit:
    kfree(bitmap_buffer);

    return result;
}

void ext2_purge_inode(struct inode* inode)
{
    ext2_instance_t* inst = (ext2_instance_t*)inode->sb->fs_info;
    
    // Чтение удаляемой иноды
    ext2_inode_t* ext2_rem_inode = new_ext2_inode();
    ext2_inode(inst, ext2_rem_inode, inode->inode);

    // Обнуление ссылок
    ext2_rem_inode->hard_links = 0;
    ext2_rem_inode->mode = 0;

    if (((inode->mode & INODE_TYPE_MASK) == INODE_FLAG_SYMLINK) && inode->size <= 60) 
    {
        // Если мы удаляем символическую ссылку и длина ее содержимого меньше 60 байт
        // Значит путь ссылки записан напрямую в адреса блоков иноды
        // Значит мы не должны пытаться очищать занятые инодой блоки
        // Так как скорее всего нарушим состояние файловой системы 
        goto skip_block_purge;
    }

    // Освобождение direct блоков
    for (int block_i = 0; block_i < EXT2_DIRECT_BLOCKS; block_i ++) {
        uint32_t block_idx = ext2_rem_inode->blocks[block_i];

        if (block_idx > 0) {
            ext2_free_block(inst, block_idx);
            ext2_rem_inode->blocks[block_i] = 0;
        }
    }

    // Освобождение indirect блоков
    ext2_free_block_tree(inst, ext2_rem_inode->blocks[EXT2_DIRECT_BLOCKS], 0);
    ext2_rem_inode->blocks[EXT2_DIRECT_BLOCKS] = 0;
    ext2_free_block_tree(inst, ext2_rem_inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1);
    ext2_rem_inode->blocks[EXT2_DIRECT_BLOCKS + 1] = 0;
    ext2_free_block_tree(inst, ext2_rem_inode->blocks[EXT2_DIRECT_BLOCKS + 2], 2);
    ext2_rem_inode->blocks[EXT2_DIRECT_BLOCKS + 2] = 0;

skip_block_purge:
    // Обнуление размера и кол-ва занимаемых блоков
    ext2_rem_inode->size = 0;
    ext2_rem_inode->num_blocks = 0;

    // Запишем deletion time, равный текущему времени
    struct timeval current_time;
    sys_get_time_epoch(&current_time);
    ext2_rem_inode->dtime = current_time.tv_sec;

    // Перезапись
    ext2_write_inode_metadata(inst, ext2_rem_inode, inode->inode);

    // Этап освобождения номера в BGD и суперблоке
    uint32_t inode_real_idx = inode->inode - 1;
    uint32_t inode_group_idx = inode_real_idx / inst->superblock->inodes_per_group;
    uint32_t inode_bitmap_idx = inode_real_idx % inst->superblock->inodes_per_group;

    // Выделение памяти под временный буфер для блока
    uint8_t* buffer = (uint8_t*) kmalloc(inst->block_size);

    // Обновить битмапу с inodes
    ext2_partition_read_block_virt(inst, inst->bgds[inode_group_idx].inode_bitmap, 1, buffer);

    buffer[inode_bitmap_idx / 8] &= ~(1 << (inode_bitmap_idx % 8));

    ext2_partition_write_block_virt(inst, inst->bgds[inode_group_idx].inode_bitmap, 1, buffer);

    // Увеличить количество свободных инодов в группе
    inst->bgds[inode_group_idx].free_inodes++;

    // Увеличить количество свободных инодов в суперблоке
    inst->superblock->free_inodes++;

    // Перезаписать на диск BGD и суперблок
    ext2_write_bgds(inst);
    ext2_rewrite_superblock(inst);

exit:
    kfree(ext2_rem_inode);
    kfree(buffer);
}

uint64_t ext2_alloc_block(ext2_instance_t* inst)
{
    uint64_t bitmap_index = 0;
    uint64_t block_index = 0;
    uint64_t group_index = 0;

    // Количество блоков в группе
    uint32_t blocks_per_group = inst->superblock->blocks_per_group;

    // Выделить временную память под буфер
    uint8_t* buffer = (uint8_t*)kmalloc(inst->block_size);

    for (uint64_t bgd_i = 0; bgd_i < inst->bgds_blocks; bgd_i ++) {
        if (inst->bgds[bgd_i].free_blocks > 0) {

            bitmap_index = 0;

            ext2_partition_read_block_virt(inst, inst->bgds[bgd_i].block_bitmap, 1, buffer);

            while ( ( buffer[bitmap_index / 8] & (1U << (bitmap_index % 8)) ) > 0) {
                bitmap_index++;
            }

            if (bitmap_index >= blocks_per_group) {
                continue;
            }

            group_index = bgd_i;
            block_index = bgd_i * blocks_per_group + bitmap_index;
            break;
        }
    }

    if (block_index == 0) {
        // Не удалось найти свободный блок
        goto exit;
    }

    // Пометить блок в битмапе как занятый
    buffer[bitmap_index / 8] |= (1 << (bitmap_index % 8));
    ext2_partition_write_block_virt(inst, inst->bgds[group_index].block_bitmap, 1, buffer);

    // Уменьшить количество свободных блоков в группе и записать на диск
    inst->bgds[group_index].free_blocks--;
    ext2_write_bgds(inst);

    // Уменьшить количество свободных блоков в суперблоке и записать на диск
    inst->superblock->free_blocks--;
    ext2_rewrite_superblock(inst);

    memset(buffer, 0, inst->block_size);
    ext2_partition_write_block_virt(inst, block_index, 1, buffer);

exit:
    kfree(buffer);
    return block_index;
}

int ext2_free_block_tree(ext2_instance_t* inst, uint64_t root, int level)
{
    if (root == 0) {
        return 0;
    }

    // Подготовить буфер для основного блока
    uint32* block_list = kmalloc(inst->block_size);

    // Cчитать блок
    ext2_partition_read_block_virt(inst, root, 1, block_list);

    for (size_t i = 0; i < inst->block_size / sizeof(uint32_t); i ++) {
        uint32_t subblock_idx = block_list[i];

        if (subblock_idx > 0) {
            if (level > 0) {
                ext2_free_block_tree(inst, subblock_idx, level - 1);
            } else {
                ext2_free_block(inst, subblock_idx);
            }
        }
    }

    memset(block_list, 0, inst->block_size);
    ext2_partition_write_block_virt(inst, root, 1, block_list);

    ext2_free_block(inst, root);
    kfree(block_list);
    return 0;
}

int ext2_free_block(ext2_instance_t* inst, uint64_t block_idx)
{
    uint32_t block_bitmap_idx = (block_idx % inst->superblock->blocks_per_group);
    uint64_t block_bgd_idx = (block_idx / inst->superblock->blocks_per_group);

    uint8_t* buffer = (uint8_t*) kmalloc(inst->block_size);

    // Считать блок с BGD
    ext2_partition_read_block_virt(inst, inst->bgds[block_bgd_idx].block_bitmap, 1, buffer);

    buffer[block_bitmap_idx / 8] &= ~(1 << (block_bitmap_idx % 8));

    // Записать измененный блок с BGD
    ext2_partition_write_block_virt(inst, inst->bgds[block_bgd_idx].block_bitmap, 1, buffer);

    // Увеличить количество свободных блоков в группе
    inst->bgds[block_bgd_idx].free_blocks++;

    // Увеличить количество свободных блоков в суперблоке
    inst->superblock->free_blocks++;

exit:
    kfree(buffer);
    return 0;
}

int ext2_alloc_inode_block(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t node_num, uint32_t block_index)
{
    // Выделить блок
    uint64_t new_block = ext2_alloc_block(inst);
    if (!new_block) {
        return -1;
    }

    // Добавим блок к inode
    ext2_inode_add_block(inst, inode, node_num, new_block, block_index);

    // Обновим количество блоков, занимаемых inode
    //uint32_t blocks_count = (block_index + 1) * (inst->block_size / 512);
    //if (inode->num_blocks < blocks_count) {
    //    inode->num_blocks = blocks_count;
    //}
    inode->num_blocks += (inst->block_size / 512);

    // Перезаписать inode
    ext2_write_inode_metadata(inst, inode, node_num);

    return 0;
}

int ext2_create_dentry(ext2_instance_t* inst, struct inode* parent, const char* name, uint32_t inode, int type)
{
    // Считать родительскую inode
    ext2_inode_t*    e2_inode = new_ext2_inode();
    ext2_inode(inst, e2_inode, parent->inode);

    if (((e2_inode->mode & EXT2_DT_CHR) == 0) || (name == NULL)) {
        kfree(e2_inode);
		return -1;
	}

    // Необходимый размер записи
    size_t dirent_len = sizeof(ext2_direntry_t) + strlen(name);
    // Выравнивание по 4 байта
    dirent_len += (dirent_len % 4) ? (4 - (dirent_len % 4)) : 0;

    // Подготовить буфер для блоков
    char* buffer = kmalloc(inst->block_size);

    uint64_t offset = 0;        // смещение относительно данных inode
    uint64_t in_block_offset = 0;
    uint64_t block_index = 0;

    int modify = -1;
    ext2_direntry_t* previous;

    ext2_read_inode_block_virt(inst, e2_inode, block_index, buffer);

    while (offset < e2_inode->size) {
        // dirent выравнены по размеру блока
        // Надо ли считать следующий блок
        if (in_block_offset >= inst->block_size) {
            // Считать следующий блок
            block_index++;
            in_block_offset -= inst->block_size;
            ext2_read_inode_block_virt(inst, e2_inode, block_index, buffer);
        }

        ext2_direntry_t* direntry = (ext2_direntry_t*)(buffer + in_block_offset);
        size_t curr_direntry_len = sizeof(ext2_direntry_t) + direntry->name_len;
        curr_direntry_len += (curr_direntry_len % 4) ? (4 - (curr_direntry_len % 4)) : 0;

        if (direntry->size != curr_direntry_len && offset + direntry->size == e2_inode->size) {
            // Добрались до конца, значит будем добавлять новую dentry
            in_block_offset += curr_direntry_len;
            offset += curr_direntry_len;

            modify = 1;
            previous = direntry;

            break;
        }

        in_block_offset += direntry->size;
        offset += direntry->size;
    }

    if (modify == 1) {
        // добавляем новую dentry
        if (in_block_offset + dirent_len >= inst->block_size) {
            // Не хватает места в inode - расширяем
            block_index++;
            uint32_t new_block_index = ext2_alloc_inode_block(inst, e2_inode, parent->inode, block_index);
            
            in_block_offset = 0;

            // Добавить размер inode и перезаписать
            e2_inode->size += inst->block_size;
            ext2_write_inode_metadata(inst, e2_inode, parent->inode);

            memset(buffer, 0, inst->block_size);
        } else {
            // последняя dentry занимает остаток блока - сожмем её
            uint32_t dentry_new_size = previous->name_len + sizeof(ext2_direntry_t);
            dentry_new_size += (dentry_new_size % 4) ? (4 - (dentry_new_size % 4)) : 0;
            previous->size = dentry_new_size;
        }
    }

    ext2_direntry_t* new_direntry = (ext2_direntry_t*)(buffer + in_block_offset);
	new_direntry->inode = inode;
	new_direntry->size = inst->block_size - in_block_offset;
	new_direntry->name_len = strlen(name);
	new_direntry->type = type;
	memcpy(new_direntry->name, name, strlen(name));
    // записать блок
	ext2_write_inode_block_virt(inst, e2_inode, parent->inode, block_index, buffer);

    kfree(buffer);
    kfree(e2_inode);

    return 1;
}

ssize_t read_inode_filedata(ext2_instance_t* inst, ext2_inode_t* inode, off_t offset, size_t size, char * buf)
{
    //Защита от выхода за границы файла
    off_t end_offset = (inode->size >= offset + size) ? (offset + size) : (inode->size);
    //Номер начального блока ноды
    off_t start_inode_block = offset / inst->block_size;
    //Смещение в начальном блоке
    off_t start_block_offset = offset % inst->block_size; 
    //Номер последнего блока
    off_t end_inode_block = end_offset / inst->block_size;
    //сколько байт взять из последнего блока
    off_t end_size = end_offset - end_inode_block * inst->block_size;

    char* temp_buffer = (char*)kmalloc(inst->block_size);
    off_t current_offset = 0;

    for (off_t block_i = start_inode_block; block_i <= end_inode_block; block_i ++) {
        off_t left_offset = (block_i == start_inode_block) ? start_block_offset : 0;
        off_t right_offset = (block_i == end_inode_block) ? (end_size - 1) : (inst->block_size - 1);
        ext2_read_inode_block_virt(inst, inode, block_i, temp_buffer);

        memcpy(buf + current_offset, temp_buffer + left_offset, (right_offset - left_offset + 1));
        current_offset += (right_offset - left_offset + 1);
    }

    kfree(temp_buffer);
    return end_offset - offset;
}

size_t ext2_inode_get_size(ext2_instance_t* inst, ext2_inode_t* inode)
{
    if (inst->file_size_64bit_flag) {
        return (size_t) inode->size_high | inode->size;
    }

    return inode->size;
}

void ext2_inode_set_size(ext2_instance_t* inst, ext2_inode_t* inode, size_t size)
{
    if (inst->file_size_64bit_flag) {
        inode->size_high = (size >> 32) & 0xFFFFFFFF;
    }

    inode->size = size & 0xFFFFFFFF;
}

// Записать данные файла inode
ssize_t write_inode_filedata(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_num, off_t offset, size_t size, const char * buf)
{
    // Вычислить необходимый размер файла
    size_t write_end = offset + size;

    // Проверить размер иноды, при необходимости расширить её
    if (write_end > ext2_inode_get_size(inst, inode)) {
        ext2_inode_set_size(inst, inode, write_end);
        ext2_write_inode_metadata(inst, inode, inode_num);
    }

    //Защита от выхода за границы файла в 32х битном режиме
    if (write_end > ext2_inode_get_size(inst, inode)) {
        return -1;
    }

    //Номер начального блока ноды
    off_t start_inode_block = offset / inst->block_size;
    //Смещение в начальном блоке
    off_t start_block_offset = offset % inst->block_size; 
    //Номер последнего блока
    off_t end_inode_block = write_end / inst->block_size;
    //сколько байт взять из последнего блока
    off_t end_size = write_end - end_inode_block * inst->block_size;

    char* temp_buffer = kmalloc(inst->block_size);

    if (start_inode_block == end_inode_block) {
        // Размер данных входит в 1 блок
        ext2_read_inode_block_virt(inst, inode, start_inode_block, temp_buffer);
        memcpy(temp_buffer + start_block_offset, buf, size);
        ext2_write_inode_block_virt(inst, inode, inode_num, start_inode_block, temp_buffer);
        goto exit;
    } else {
        
        uint32_t written_blocks = 0;

        for (uint32_t block_i = start_inode_block; block_i < end_inode_block; block_i ++, written_blocks++) {
            // Зануляем память
            memset(temp_buffer, 0, inst->block_size);
            // Смещение буфера
            size_t buffer_offset = written_blocks * inst->block_size - start_block_offset;

            if (block_i == start_inode_block) {
                // Возможно, у нас есть неполный блок в начале
                // Считаем блок
                ext2_read_inode_block_virt(inst, inode, start_inode_block, temp_buffer);
                // Скопируем новую часть блока
                memcpy(temp_buffer + start_block_offset, buf, inst->block_size - start_block_offset);
                // Запишем на диск
                ext2_write_inode_block_virt(inst, inode, inode_num, start_inode_block, temp_buffer);
            } else {
                // Скопировать в буфер данные для записи
                memcpy(temp_buffer, buf + buffer_offset, inst->block_size);
                // Запишем на диск
                ext2_write_inode_block_virt(inst, inode, inode_num, block_i, temp_buffer);
            }
        }

        if (end_size > 0) {
            // Есть остатки данных
            // Считать блок
            ext2_read_inode_block_virt(inst, inode, end_inode_block, temp_buffer);
            // Скопировать в буфер данные для записи
            size_t buffer_offset = written_blocks * inst->block_size - start_block_offset;
            memcpy(temp_buffer, buf + buffer_offset, end_size);
            // Запишем на диск
            ext2_write_inode_block_virt(inst, inode, inode_num, end_inode_block, temp_buffer);
        }
    }

exit:
    ext2_write_bgds(inst);
    kfree(temp_buffer);
    return size;
}

void ext2_inode(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t node_index)
{
    node_index--;
    //Группа, к которой принадлежит инода с указанным индексом
    uint32_t inode_group = node_index / inst->superblock->inodes_per_group;
    //Блок, указывающий на таблицу нод данной группы
    uint32_t inode_table_block = inst->bgds[inode_group].inode_table;
    // Индекс в данной группе
    uint32_t idx_in_group = node_index - inode_group * inst->superblock->inodes_per_group;

    uint32_t block_offset = idx_in_group * inst->superblock->inode_size / inst->block_size;
    uint32_t offset_in_block = idx_in_group - block_offset * (inst->block_size / inst->superblock->inode_size);

    // Считать данные inode
    char* buffer = kmalloc(inst->block_size);
    ext2_partition_read_block_virt(inst, inode_table_block + block_offset, 1, buffer);
    memcpy(inode, buffer + offset_in_block * inst->superblock->inode_size, sizeof(ext2_inode_t));
    //Освободить временный буфер
    kfree(buffer);
}

void ext2_write_inode_metadata(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t node_index)
{
    node_index--;
    //Группа, к которой принадлежит инода с указанным индексом
    uint32_t inode_group = node_index / inst->superblock->inodes_per_group;
    //Блок, указывающий на таблицу нод данной группы
    uint32_t inode_table_block = inst->bgds[inode_group].inode_table;
    // Индекс в данной группе
    uint32_t idx_in_group = node_index - inode_group * inst->superblock->inodes_per_group;

    uint32_t block_offset = idx_in_group * inst->superblock->inode_size / inst->block_size;
    uint32_t offset_in_block = idx_in_group - block_offset * (inst->block_size / inst->superblock->inode_size);

    // Считать целый блок
    char* buffer = kmalloc(inst->block_size);
    ext2_partition_read_block_virt(inst, inode_table_block + block_offset, 1, buffer);
    // Заменить в нем область байт данной inode
    memcpy(buffer + offset_in_block * inst->superblock->inode_size, inode, sizeof(ext2_inode_t));
    // Записать измененный блок обратно на диск
    ext2_partition_write_block_virt(inst, inode_table_block + block_offset, 1, buffer);
    //Освободить временный буфер
    kfree(buffer);
}

struct inode* ext2_read_node(struct superblock* sb, uint64_t ino_num)
{
    ext2_instance_t* inst = (ext2_instance_t*)sb->fs_info;
    ext2_inode_t* e2_inode = new_ext2_inode();
    ext2_inode(inst, e2_inode, ino_num);
    struct inode* vfs_inode = ext2_inode_to_vfs_inode(inst, e2_inode, ino_num);
    kfree(e2_inode);
    return vfs_inode;
}

struct dirent* ext2_dirent_to_vfs_dirent(ext2_direntry_t* ext2_dirent)
{
    struct dirent* result = new_vfs_dirent();
    result->inode = ext2_dirent->inode;
    strncpy(result->name, ext2_dirent->name, ext2_dirent->name_len);
    result->type = ext2_dt_to_vfs_dt(ext2_dirent->type);

    return result;
}

int ext2_dt_to_vfs_dt(int ext2_dt)
{
    int dt = 0;
    switch (ext2_dt) {
        case EXT2_DT_REG:
            dt = DT_REG;
            break;
        case EXT2_DT_SYMLINK:
            dt = DT_LNK;
            break;
        case EXT2_DT_DIR:
            dt = DT_DIR;
            break;
        case EXT2_DT_CHR:
            dt = DT_CHR;
            break;
        case EXT2_DT_FIFO:
            dt = DT_FIFO;
            break;
        case EXT2_DT_BLK:
            dt = DT_BLK;
            break;
        case EXT2_DT_SOCK:
            dt = DT_SOCK;
            break;
    }

    return dt;
}

int vfs_inode_mode_to_ext2_dentry_type(int inode_mode)
{
    int dt = 0;
    switch (inode_mode & INODE_TYPE_MASK) 
    {
        case INODE_TYPE_FILE:
            dt = EXT2_DT_REG;
            break;
        case INODE_FLAG_SYMLINK:
            dt = EXT2_DT_SYMLINK;
            break;
        case INODE_TYPE_DIRECTORY:
            dt = EXT2_DT_DIR;
            break;
        case INODE_FLAG_CHARDEVICE:
            dt = EXT2_DT_CHR;
            break;
        case INODE_FLAG_PIPE:
            dt = EXT2_DT_FIFO;
            break;
        case INODE_FLAG_BLOCKDEVICE:
            dt = EXT2_DT_BLK;
            break;
        case INODE_FLAG_SOCKET:
            dt = EXT2_DT_SOCK;
            break;
    }

    return dt;
}

struct inode* ext2_inode_to_vfs_inode(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t ino_num)
{
    struct inode* result = new_vfs_inode();
    result->inode = ino_num;
    result->sb = inst->vfs_sb;
    result->uid = inode->userid;
    result->gid = inode->gid;
    result->size = inode->size;
    result->mode = inode->mode;
    result->access_time = inode->atime;
    result->create_time = inode->ctime;
    result->modify_time = inode->mtime;
    result->hard_links = inode->hard_links;
    result->device = (dev_t) inst;

    if ((inode->mode & INODE_TYPE_MASK) == INODE_TYPE_FILE) {
        
        if (inst->file_size_64bit_flag) {
            // Файловая система поддерживает 64-х битный размер файла, пересчитаем
            result->size = (uint64_t)inode->size_high << 32 | result->size;
        }

        result->operations = &file_inode_ops;
        result->file_ops = &file_ops;
    }

    if ((inode->mode & INODE_TYPE_MASK) == INODE_TYPE_DIRECTORY) {
        result->operations = &dir_inode_ops;
        result->file_ops = &dir_ops;
    }
    if ((inode->mode & INODE_TYPE_MASK) == INODE_FLAG_SYMLINK) {
        result->operations = &symlink_inode_ops;
    }
    if ((inode->mode & INODE_TYPE_MASK) == INODE_FLAG_BLOCKDEVICE) {
    
    }

    return result;
}

ssize_t ext2_file_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct inode* inode = file->inode;
    
    ext2_instance_t* inst = (ext2_instance_t*)inode->sb->fs_info;
    ext2_inode_t*    e2_inode = new_ext2_inode();
    ext2_inode(inst, e2_inode, inode->inode);
    
    ssize_t result = read_inode_filedata(inst, e2_inode, offset, count, buffer);
    file->pos += result;

    kfree(e2_inode);
    
    return result;
}

ssize_t ext2_file_write(struct file* file, const char* buffer, size_t count, loff_t offset)
{
    struct inode* inode = file->inode;

    ext2_instance_t* inst = (ext2_instance_t*)inode->sb->fs_info;
    ext2_inode_t*    e2_inode = new_ext2_inode();
    ext2_inode(inst, e2_inode, inode->inode);

    ssize_t result = write_inode_filedata(inst, e2_inode, inode->inode, offset, count, buffer);
    file->pos += result;

    inode->size = e2_inode->size;
    inode->blocks = e2_inode->num_blocks;

    kfree(e2_inode);

    return result;
}

int ext2_truncate(struct inode* inode)
{
    ext2_instance_t* inst = (ext2_instance_t*)inode->sb->fs_info;
    ext2_inode_t* e2_inode = new_ext2_inode();
    ext2_inode(inst, e2_inode, inode->inode);
    
    // Освобождение direct блоков
    for (int block_i = 0; block_i < EXT2_DIRECT_BLOCKS; block_i ++) {
        uint32_t block_idx = e2_inode->blocks[block_i];

        if (block_idx > 0) {
            ext2_free_block(inst, block_idx);
            e2_inode->blocks[block_i] = 0;
        }
    }

    // Освобождение indirect блоков
    ext2_free_block_tree(inst, e2_inode->blocks[EXT2_DIRECT_BLOCKS], 0);
    e2_inode->blocks[EXT2_DIRECT_BLOCKS] = 0;
    ext2_free_block_tree(inst, e2_inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1);
    e2_inode->blocks[EXT2_DIRECT_BLOCKS + 1] = 0;
    ext2_free_block_tree(inst, e2_inode->blocks[EXT2_DIRECT_BLOCKS + 2], 2);
    e2_inode->blocks[EXT2_DIRECT_BLOCKS + 2] = 0;

    // Обнуление размера и кол-ва занимаемых блоков
    e2_inode->size = 0;
    e2_inode->num_blocks = 0;

    inode->size = 0;
    inode->blocks = 0;

    ext2_write_inode_metadata(inst, e2_inode, inode->inode);
    kfree(e2_inode);

    return 0;
}

int ext2_rename(struct inode* oldparent, struct dentry* orig_dentry, struct inode* newparent, const char* newname)
{
    ext2_instance_t* oldpinst = (ext2_instance_t*)oldparent->sb->fs_info;
    ext2_instance_t* newpinst = (ext2_instance_t*)newparent->sb->fs_info;

    if (oldpinst != newpinst) {
        return -ERROR_OTHER_DEVICE;
    }

    if (ext2_find_dentry(oldpinst->vfs_sb, newparent, newname, NULL) != WRONG_INODE_INDEX) {
        return -ERROR_ALREADY_EXISTS;
    }

    int orig_type = 0;
    int rc = ext2_remove_dentry(oldpinst, oldparent, orig_dentry->name, &orig_type);
    if (rc != 0)
    {
        // Почему-то не смогли удалить dentry из старого родителя - лучше выйти с ошибкой
        return -ENOENT;
    }

    ext2_create_dentry(oldpinst, newparent, newname, orig_dentry->d_inode->inode, orig_type);

    return 0;
}

int ext2_chmod(struct inode * inode, uint32_t mode)
{
    ext2_instance_t* inst = (ext2_instance_t*)inode->sb->fs_info;
    ext2_inode_t* e2_inode = new_ext2_inode();
    ext2_inode(inst, e2_inode, inode->inode);
    e2_inode->mode = (e2_inode->mode & 0xFFFFF000) | mode;
    ext2_write_inode_metadata(inst, e2_inode, inode->inode);
    kfree(e2_inode);

    return 0;
}

int ext2_unlink(struct inode* parent, struct dentry* dent)
{
    ext2_instance_t* inst = (ext2_instance_t*)parent->sb->fs_info;

    // Считать ext2 - inode директории, из которой удаляем
    ext2_inode_t* e2_parent_inode = new_ext2_inode();
    ext2_inode(inst, e2_parent_inode, parent->inode);

    // Получить удаляемую inode
    uint32_t unlink_inode_idx = 0;
    if ((unlink_inode_idx = ext2_find_dentry(parent->sb, parent, dent->name, NULL)) == WRONG_INODE_INDEX) {
        kfree(e2_parent_inode);
        return -ERROR_NO_FILE;
    }

    // Считать ext2 - inode, для которой делаем unlink
    ext2_inode_t* e2_inode = new_ext2_inode();
    ext2_inode(inst, e2_inode, unlink_inode_idx);

    //printk("UNLINK: INODE %i SIZE %i BLOCKS %i\n", unlink_inode_idx, e2_inode->size, e2_inode->num_blocks);

    // Удалить dentry из родительской inode
    if (ext2_remove_dentry(inst, parent, dent->name, NULL) != 0) {
        goto exit;
    }

    // Уменьшить количество ссылок на выбранную inode
    e2_inode->hard_links--;

    // Перезаписать inode
    ext2_write_inode_metadata(inst, e2_inode, unlink_inode_idx);

    // Уменьшить количество ссылок на выбранную inode в VFS
    dent->d_inode->hard_links--;

exit:
    kfree(e2_parent_inode);
    kfree(e2_inode);

    return 0;
}

int ext2_rmdir(struct inode* inode, struct dentry* dentry)
{
    int rc = 0;

    ext2_instance_t* inst = (ext2_instance_t*) inode->sb->fs_info;

    if (dentry->d_inode->hard_links > 2) {
        rc = -ENOTEMPTY;
        goto exit;
    }

    //Получить ext2 иноду удаляемой директории
    ext2_inode_t* dir_inode = new_ext2_inode();
    ext2_inode(inst, dir_inode, dentry->d_inode->inode);

    // ------ Проверяем директорию, чтобы она была пустой ------------------
    //Переменные
    int dot = 0, dotdot = 0, other = 0;
    uint32_t curr_offset = 0;
    uint32_t block_offset = 0;
    uint32_t in_block_offset = 0;
    //Выделить временную память под буфер блоков
    char* buffer = kmalloc(inst->block_size);
    //Прочитать начальный блок иноды
    ext2_read_inode_block_virt(inst, dir_inode, block_offset, buffer);
    //Проверка, не прочитан ли весь блок?
    while (curr_offset < dir_inode->size) {

        if (in_block_offset >= inst->block_size) {
            block_offset++;
            in_block_offset = 0;
            ext2_read_inode_block_virt(inst, dir_inode, block_offset, buffer);
        }

        ext2_direntry_t* curr_entry = (ext2_direntry_t*)(buffer + in_block_offset);

        if (curr_entry->inode != 0) {
            if (curr_entry->name[0] == '.' && curr_entry->name_len == 1) {
                dot = 1;
            } else if (curr_entry->name[0] == '.' && curr_entry->name[1] == '.' && curr_entry->name_len == 2) {
                dotdot = 1;
            } else {
                other ++;
            }
        }

        in_block_offset += curr_entry->size;
        curr_offset += curr_entry->size;
    }
    kfree(buffer);
    // Должны быть только . и ..
    if (!dot || !dotdot || other > 0) {
        rc = -ENOTEMPTY;
        goto exit;
    }

    // Удалить dentry директории из родительской inode
    if (ext2_remove_dentry(inst, inode, dentry->name, NULL) != 0) {
        goto exit;
    }

    // Обнулить количество ссылок на выбранную inode
    dir_inode->hard_links = 0;
    // Перезаписать удаляемую inode
    ext2_write_inode_metadata(inst, dir_inode, dentry->d_inode->inode);
    // Обнулить количество ссылок на выбранную inode в VFS
    dentry->d_inode->hard_links = 0;
    
    // Также должны уменьшить кол-во ссылок родительской директории, т.к в удаляемой директории есть ..
    // Считать ext2 - inode директории, из которой удаляем
    ext2_inode_t* parent_inode = new_ext2_inode();
    ext2_inode(inst, parent_inode, inode->inode);

    parent_inode->hard_links--;
    // Перезаписать inode - родителя
    ext2_write_inode_metadata(inst, parent_inode, inode->inode);
    // Уменьшить количество ссылок на родительскую inode в VFS
    inode->hard_links --;

    // Обновить информацию о свободных папках в bgds
    uint32_t inode_group = (dentry->d_inode->inode - 1) / inst->superblock->inodes_per_group;
	inst->bgds[inode_group].used_dirs--;
	ext2_write_bgds(inst);
    
    // Освобождение
    kfree(parent_inode);

exit:
    kfree(dir_inode);
    return rc;
}

int ext2_mkdir(struct inode* parent, const char* dir_name, uint32_t mode)
{
    ext2_instance_t* inst = (ext2_instance_t*)parent->sb->fs_info;
    
    if (ext2_find_dentry(parent->sb, parent, dir_name, NULL) != WRONG_INODE_INDEX) {
        return -ERROR_ALREADY_EXISTS;
    }

    // Создать inode на диске
    uint32_t inode_num = ext2_alloc_inode(inst);
    if (inode_num == 0) {
        return -ERROR_NO_SPACE;
    }
    // Прочитать inode
    ext2_inode_t *inode = new_ext2_inode();
    ext2_inode(inst, inode, inode_num);

    struct timeval current_time;
    sys_get_time_epoch(&current_time);

    inode->mode = INODE_TYPE_DIRECTORY | (0xFFF & mode);
    inode->atime = current_time.tv_sec;
    inode->ctime = current_time.tv_sec;
    inode->mtime = current_time.tv_sec;
    inode->dtime = 0;
    inode->gid = 0;
    inode->userid = 0;
    inode->flags = 0;
    inode->hard_links = 2;
    inode->num_blocks = 0;
    inode->os_specific1 = 0;
    memset(inode->blocks, 0, sizeof(inode->blocks));
    memset(inode->os_specific2, 0, 12);
    inode->generation = 0;
    inode->file_acl = 0;
    inode->dir_acl = 0;
    inode->faddr = 0;
    inode->size = inst->block_size;

    // Записать изменения на диск
    ext2_write_inode_metadata(inst, inode, inode_num);

    ext2_alloc_inode_block(inst, inode, inode_num, 0);

    // Добавить иноду в директорию
    ext2_create_dentry(inst, parent, dir_name, inode_num, EXT2_DT_DIR);

    // Добавить . и .. в новую директорию
    char* block_temp = kmalloc(inst->block_size);
    memset(block_temp, 0, inst->block_size);
    // .
    ext2_direntry_t* dot_direntry = (ext2_direntry_t*)block_temp;
    dot_direntry->inode = inode_num;
	dot_direntry->size = 12;
	dot_direntry->name_len = 1;
    dot_direntry->type = EXT2_DT_DIR;
	dot_direntry->name[0] = '.';
    // ..
    dot_direntry = (ext2_direntry_t*)(block_temp + 12);
    dot_direntry->inode = parent->inode;
	dot_direntry->size = inst->block_size - 12;
	dot_direntry->name_len = 2;
    dot_direntry->type = EXT2_DT_DIR;
	dot_direntry->name[0] = '.';
    dot_direntry->name[1] = '.';

    // записать блок в inode
    ext2_write_inode_block_virt(inst, inode, inode_num, 0, block_temp);
    kfree(block_temp);

    // Увеличить количество ссылок родительской иноды
    ext2_inode(inst, inode, parent->inode);
    inode->hard_links++;
    ext2_write_inode_metadata(inst, inode, parent->inode);

    // Увеличить количество ссылок vfs inode родителя
    parent->hard_links++;

    // Обновить информацию о свободных папках в bgds
    uint32_t new_inode_group = (inode_num - 1) / inst->superblock->inodes_per_group;
	inst->bgds[new_inode_group].used_dirs++;
	ext2_write_bgds(inst);

    kfree(inode);
    return 0;
}

int ext2_mkfile(struct inode* parent, const char* file_name, uint32_t mode)
{
    ext2_instance_t* inst = (ext2_instance_t*)parent->sb->fs_info;

    if (ext2_find_dentry(parent->sb, parent, file_name, NULL) != WRONG_INODE_INDEX) {
        return -ERROR_ALREADY_EXISTS;
    }

    // Создать inode на диске
    uint32_t inode_num = ext2_alloc_inode(inst);
    if (inode_num == 0) {
        return -ERROR_NO_SPACE;
    }
    // Прочитать inode
    ext2_inode_t *inode = new_ext2_inode();
    ext2_inode(inst, inode, inode_num);

    struct timeval current_time;
    sys_get_time_epoch(&current_time);

    inode->mode = INODE_TYPE_FILE | (0xFFF & mode);
    inode->atime = current_time.tv_sec;
    inode->ctime = current_time.tv_sec;
    inode->mtime = current_time.tv_sec;
    inode->dtime = 0;
    inode->gid = 0;
    inode->userid = 0;
    inode->flags = 0;
    inode->hard_links = 1;
    inode->num_blocks = 0;
    inode->os_specific1 = 0;
    memset(inode->blocks, 0, sizeof(inode->blocks));
    memset(inode->os_specific2, 0, sizeof(inode->os_specific2));
    inode->generation = 0;
    inode->file_acl = 0;
    inode->dir_acl = 0;
    inode->faddr = 0;
    inode->size = 0;

    // Записать изменения на диск
    ext2_write_inode_metadata(inst, inode, inode_num);

    // Добавить иноду в директорию
    ext2_create_dentry(inst, parent, file_name, inode_num, EXT2_DT_REG);

    kfree(inode);
    return 0;
}

int ext2_mknod (struct inode* parent, const char* name, mode_t mode)
{
    ext2_instance_t* inst = (ext2_instance_t*)parent->sb->fs_info;

    if (ext2_find_dentry(parent->sb, parent, name, NULL) != WRONG_INODE_INDEX) {
        return -ERROR_ALREADY_EXISTS;
    }

    // Создать inode на диске
    uint32_t inode_num = ext2_alloc_inode(inst);
    if (inode_num == 0) {
        return -ERROR_NO_SPACE;
    }
    // Прочитать inode
    ext2_inode_t *inode = new_ext2_inode();
    ext2_inode(inst, inode, inode_num);

    struct timeval current_time;
    sys_get_time_epoch(&current_time);

    inode->mode = mode;
    inode->atime = current_time.tv_sec;
    inode->ctime = current_time.tv_sec;
    inode->mtime = current_time.tv_sec;
    inode->dtime = 0;
    inode->gid = 0;
    inode->userid = 0;
    inode->flags = 0;
    inode->hard_links = 1;
    inode->num_blocks = 0;
    inode->os_specific1 = 0;
    memset(inode->blocks, 0, sizeof(inode->blocks));
    memset(inode->os_specific2, 0, sizeof(inode->os_specific2));
    inode->generation = 0;
    inode->file_acl = 0;
    inode->dir_acl = 0;
    inode->faddr = 0;
    inode->size = 0;

    // Записать изменения на диск
    ext2_write_inode_metadata(inst, inode, inode_num);

    // Добавить иноду в директорию
    int dentry_type = vfs_inode_mode_to_ext2_dentry_type(mode);
    ext2_create_dentry(inst, parent, name, inode_num, dentry_type);

    kfree(inode);
    return 0;
}

int ext2_symlink(struct inode* parent, const char* name, const char* target)
{
    ext2_instance_t* inst = (ext2_instance_t*)parent->sb->fs_info;

    if (ext2_find_dentry(parent->sb, parent, name, NULL) != WRONG_INODE_INDEX) {
        return -ERROR_ALREADY_EXISTS;
    }

    size_t target_len = strlen(target);

    // Создать inode на диске
    uint32_t inode_num = ext2_alloc_inode(inst);
    if (inode_num == 0) {
        return -ERROR_NO_SPACE;
    }
    // Прочитать inode
    ext2_inode_t *inode = new_ext2_inode();
    ext2_inode(inst, inode, inode_num);

    struct timeval current_time;
    sys_get_time_epoch(&current_time);

    // подготовить структуру inode
    inode->mode = INODE_FLAG_SYMLINK | (0777);
    inode->atime = current_time.tv_sec;
    inode->ctime = current_time.tv_sec;
    inode->mtime = current_time.tv_sec;
    inode->dtime = 0;
    inode->gid = 0;
    inode->userid = 0;
    inode->flags = 0;
    inode->hard_links = 1;
    inode->num_blocks = 0;
    inode->os_specific1 = 0;
    memset(inode->blocks, 0, sizeof(inode->blocks));
    memset(inode->os_specific2, 0, sizeof(inode->os_specific2));
    inode->generation = 0;
    inode->file_acl = 0;
    inode->dir_acl = 0;
    inode->faddr = 0;
    inode->size = target_len;

    if (target_len <= 60)
    {
        memcpy(inode->blocks, target, target_len);
    }
    else 
    {
        write_inode_filedata(inst, inode, inode_num, 0, target_len, target);
    }

    // Записать изменения на диск
    ext2_write_inode_metadata(inst, inode, inode_num);

    // Добавить иноду в директорию
    ext2_create_dentry(inst, parent, name, inode_num, EXT2_DT_SYMLINK);

    kfree(inode);

    return 0;
}

ssize_t ext2_readlink(struct inode* inode, char* pathbuf, size_t pathbuflen)
{
    ext2_instance_t* inst = (ext2_instance_t*) inode->sb->fs_info;

    // Получить ext2 иноду ссылки
    ext2_inode_t* symlink_inode = new_ext2_inode();
    ext2_inode(inst, symlink_inode, inode->inode);

    // Размер данных, который будем считывать
    ssize_t readable = MIN(symlink_inode->size, pathbuflen);

    if (symlink_inode->size <= 60)
    {
        memcpy(pathbuf, symlink_inode->blocks, readable);
    } 
    else 
    {
        read_inode_filedata(inst, symlink_inode, 0, readable, pathbuf);
    }

    kfree(symlink_inode);
    return readable;
}

int ext2_linkat (struct dentry* src, struct inode* dst_dir, const char* dst_name)
{
    ext2_instance_t* inst = (ext2_instance_t*) dst_dir->sb->fs_info;

    if (ext2_find_dentry(dst_dir->sb, dst_dir, dst_name, NULL) != WRONG_INODE_INDEX) 
    {
        return -ERROR_ALREADY_EXISTS;
    }

    struct inode* src_inode = src->d_inode;
    uint64_t inode_idx = src_inode->inode;

    // Тип dentry из типа inode
    int dentry_type = vfs_inode_mode_to_ext2_dentry_type(src_inode->mode);
    // Добавить иноду в директорию
    ext2_create_dentry(inst, dst_dir, dst_name, inode_idx, dentry_type); // TODO: correct dentry type

    // Считать ext2 - inode, для которой делаем unlink
    ext2_inode_t* e2_inode = new_ext2_inode();
    ext2_inode(inst, e2_inode, inode_idx);
    // Увеличить количество ссылок на выбранную inode
    e2_inode->hard_links++;
    // Перезаписать inode
    ext2_write_inode_metadata(inst, e2_inode, inode_idx);

    // Увеличить количество жестких ссылок объекта inode VFS
    src_inode->hard_links++;

    kfree(e2_inode);
    return 0;
}

struct dirent* ext2_file_readdir(struct file* dir, uint32_t index)
{
    struct inode* vfs_inode = dir->inode;
    struct dirent* result = NULL;
    ext2_instance_t* inst = (ext2_instance_t*)vfs_inode->sb->fs_info;
    //Получить родительскую иноду
    ext2_inode_t* parent_inode = new_ext2_inode();
    ext2_inode(inst, parent_inode, vfs_inode->inode);
    //Переменные
    uint32_t curr_offset = 0;
    uint32_t block_offset = 0;
    uint32_t in_block_offset = 0;
    uint32_t passed_entries = 0;
    //Выделить временную память под буфер блоков
    char* buffer = kmalloc(inst->block_size);
    //Прочитать начальный блок иноды
    ext2_read_inode_block_virt(inst, parent_inode, block_offset, buffer);
    //Проверка, не прочитан ли весь блок?
    while (curr_offset < parent_inode->size) {

        if (in_block_offset >= inst->block_size) {
            block_offset++;
            in_block_offset = 0;
            ext2_read_inode_block_virt(inst, parent_inode, block_offset, buffer);
        }

        ext2_direntry_t* curr_entry = (ext2_direntry_t*)(buffer + in_block_offset);

        if (curr_entry->inode != 0) {
            if (passed_entries == index) {
                result = ext2_dirent_to_vfs_dirent(curr_entry);
                goto exit;
            }

            passed_entries++;
        }

        in_block_offset += curr_entry->size;
        curr_offset += curr_entry->size;
    }

exit:

    kfree(parent_inode);
    kfree(buffer);

    return result;
}

int ext2_remove_dentry(ext2_instance_t* inst, struct inode* parent, const char* name, int* orig_type)
{
    int rc = -1;
    ext2_inode_t*    e2_inode = new_ext2_inode();
    ext2_inode(inst, e2_inode, parent->inode);

    // Подготовить буфер для блоков
    char* buffer = kmalloc(inst->block_size);
    // смещение относительно данных inode
    uint64_t offset = 0;
    uint64_t offset_in_block = 0;
    uint64_t block_index = 0;
    
    size_t name_len = strlen(name);

    // Считать первый блок
    ext2_read_inode_block_virt(inst, e2_inode, block_index, buffer);

    while (offset < e2_inode->size) {
        
        // Надо ли считать следующий блок
        if (offset_in_block >= inst->block_size) {
            // Считать следующий блок
            block_index++;
            offset_in_block = 0;
            ext2_read_inode_block_virt(inst, e2_inode, block_index, buffer);
        }

        ext2_direntry_t* curr_entry = (ext2_direntry_t*)(buffer + offset_in_block);
        // Проверка совпадения имени
        if((curr_entry->inode != 0) && (strncmp(curr_entry->name, name, curr_entry->name_len) == 0) && name_len == curr_entry->name_len) {
            // инвалидировать dentry
            curr_entry->inode = 0;
            if (orig_type != NULL) {
                *orig_type = curr_entry->type;
            }
            // Записать изменения на диск
            ext2_write_inode_block_virt(inst, e2_inode, parent->inode, block_index, buffer);
            rc = 0;
            goto exit;
        }

        offset_in_block += curr_entry->size;
        offset += curr_entry->size;
    }

exit:
    kfree(buffer);
    kfree(e2_inode);

    return rc;
}

int ext2_statfs(struct superblock *sb, struct statfs* stat)
{
    ext2_instance_t* inst = (ext2_instance_t*)sb->fs_info;

    stat->blocksize = inst->block_size;

    stat->blocks = inst->superblock->total_blocks;
    stat->blocks_free = inst->superblock->free_blocks;

    stat->inodes = inst->superblock->total_inodes;
    stat->inodes_free = inst->superblock->free_inodes;

    return 0;
}

uint64_t ext2_find_dentry(struct superblock* sb, struct inode* vfs_parent_inode, const char *name, int* type)
{
    uint64_t result = WRONG_INODE_INDEX;
    ext2_instance_t* inst = (ext2_instance_t*)sb->fs_info;
    
    size_t name_len = strlen(name);

    //Получить родительскую иноду
    ext2_inode_t* parent_inode = new_ext2_inode();
    ext2_inode(inst, parent_inode, vfs_parent_inode->inode);

    //Переменные
    uint32_t curr_offset = 0;
    uint32_t block_offset = 0;
    uint32_t in_block_offset = 0;
    
    //Выделить временную память под буфер блоков
    char* buffer = kmalloc(inst->block_size);
    
    //Прочитать начальный блок иноды
    ext2_read_inode_block_virt(inst, parent_inode, block_offset, buffer);
    //Пока не прочитаны все блоки
    while(curr_offset < parent_inode->size) {

        //Проверка, не прочитан ли весь блок?
        if(in_block_offset >= inst->block_size){
            block_offset++;
            in_block_offset = 0;
            ext2_read_inode_block_virt(inst, parent_inode, block_offset, buffer);
        }

        // Считанный dirent
        ext2_direntry_t* curr_entry = (ext2_direntry_t*)(buffer + in_block_offset);
        // Проверка совпадения имени
        if((curr_entry->inode != 0) && (strncmp(curr_entry->name, name, curr_entry->name_len) == 0) && name_len == curr_entry->name_len) {
            result = curr_entry->inode;

            if (type != NULL) {
                *type = ext2_dt_to_vfs_dt(curr_entry->type);
            }

            goto exit;
        }

        in_block_offset += curr_entry->size;
        curr_offset += curr_entry->size;
    }

exit:
    kfree(parent_inode);
    kfree(buffer);

    return result;
}