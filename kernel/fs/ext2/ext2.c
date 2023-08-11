#include "ext2.h"
#include "../vfs/filesystems.h"
#include "string.h"
#include "memory/mem_layout.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "stdio.h"

struct inode_operations file_inode_ops;
struct inode_operations dir_inode_ops;

struct file_operations file_ops;
struct file_operations dir_ops;

struct super_operations sb_ops;

void ext2_init()
{
    filesystem_t* ext2fs = new_filesystem();
    ext2fs->name = "ext2";
    ext2fs->mount = ext2_mount;

    filesystem_register(ext2fs);

    file_inode_ops.chmod = ext2_chmod;
    file_inode_ops.open = ext2_open;
    file_inode_ops.close = ext2_close;
    file_inode_ops.truncate = ext2_truncate;

    dir_inode_ops.mkdir = ext2_mkdir;
    dir_inode_ops.mkfile = ext2_mkfile;
    dir_inode_ops.chmod = ext2_chmod;
    dir_inode_ops.open = ext2_open;
    dir_inode_ops.close = ext2_close;
    //dir_inode_ops.unlink = ext2_unlink;

    file_ops.read = ext2_file_read;
    //file_ops.write = ext2_file_write;

    dir_ops.readdir = ext2_file_readdir;

    sb_ops.read_inode = ext2_read_node;
    sb_ops.find_dentry = ext2_find_dentry;
}

ext2_inode_t* new_ext2_inode()
{
    ext2_inode_t* ext2_inode = kmalloc(sizeof(ext2_inode_t));
    memset(ext2_inode, 0, sizeof(ext2_inode_t));
    return ext2_inode;
}

uint32_t ext2_partition_read_block(ext2_instance_t* inst, uint64_t block_start, uint64_t blocks, char* buffer){
    uint64_t    start_lba = block_start * (inst->block_size / 512);
    uint64_t    lba_count = blocks  * (inst->block_size / 512);
    return partition_read(inst->partition, start_lba, lba_count, buffer);
}

uint32_t ext2_partition_write_block(ext2_instance_t* inst, uint64_t block_start, uint64_t blocks, char* buffer){
    uint64_t    start_lba = block_start * inst->block_size / 512;
    uint64_t    lba_count = blocks  * inst->block_size / 512;
    return partition_write(inst->partition, start_lba, lba_count, buffer);
}

uint32_t ext2_inode_block_absolute(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_block_index)
{
    uint32_t level = inst->block_size / 4;
    int d, e, f, g;
    if(inode_block_index < EXT2_DIRECT_BLOCKS) {
        return inode->blocks[inode_block_index];
    }

    int a = inode_block_index - EXT2_DIRECT_BLOCKS;
    //Выделение временной памяти под считываемые блоки
    uint32_t* tmp = kmalloc(inst->block_size);
    //Получение физического адреса выделенной памяти
    char* tmp_phys = (char*)kheap_get_phys_address(tmp);
    int b = a - level;
    if(b < 0) {
        ext2_partition_read_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS], 1, tmp_phys);
        uint32_t result = tmp[a];
        kfree(tmp);
        return result;
    }
    int c = b - level * level;
    if(c < 0) {
        c = b / level;
        d = b - c * level;
        ext2_partition_read_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1, tmp_phys);
        ext2_partition_read_block(inst, tmp[c], 1, tmp_phys);
        uint32_t result = tmp[d];
        kfree(tmp);
        return result;
    }
    d = c - level * level * level;
    if(d < 0) {
        e = c / (level * level);
        f = (c - e * level * level) / level;
        g = (c - e * level * level - f * level);
        ext2_partition_read_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 2], 1, tmp_phys);
        ext2_partition_read_block(inst, tmp[e], 1, tmp_phys);
        ext2_partition_read_block(inst, tmp[f], 1, tmp_phys);
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
            uint64_t new_block_index = ext2_alloc_block(inst);
            
            if (!new_block_index) {
                rc = -2;
                goto exit;
            }

            inode->blocks[EXT2_DIRECT_BLOCKS] = new_block_index;
            ext2_write_inode_metadata(inst, inode, inode_index);
        }

        // Считать блок по адресу
        ext2_partition_read_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS], 1, (char*)kheap_get_phys_address(buffer));

        // Записать адрес блока в дополнительный блок
        ((uint32_t*)buffer)[inode_block - EXT2_DIRECT_BLOCKS] = abs_block;

        // Записать измененный блок
        ext2_partition_write_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS], 1, (char*)kheap_get_phys_address(buffer));
        
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
        ext2_partition_read_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1, (char*)kheap_get_phys_address(buffer));

        if (!((uint32_t*)buffer)[c]) {
            // Адрес доп. блока не задан - выделяем блок и задаем адрес
            uint64_t new_block_index = ext2_alloc_block(inst);
            if (!new_block_index) {
                rc = -2;
                goto exit;
            }
            ((uint32_t*)buffer)[c] = new_block_index;
            ext2_partition_write_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1, (char*)kheap_get_phys_address(buffer));
        }

        uint32_t saved_block = ((uint32_t*)buffer)[c];
        // Считать блок по адресу
        ext2_partition_read_block(inst, ((uint32_t*)buffer)[c], 1, (char*)kheap_get_phys_address(buffer));

        // Записать адрес блока в дополнительный блок
        ((uint32_t*)buffer)[d] = abs_block;

        // Записать измененный блок
        ext2_partition_write_block(inst, saved_block, 1, (char*)kheap_get_phys_address(buffer));
        
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
        ext2_partition_read_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 2], 1, (char*)kheap_get_phys_address(buffer));

        if (!((uint32_t*)buffer)[d]) {
            // Адрес доп. блока не задан - выделяем блок и задаем адрес
            uint64_t new_block_index = ext2_alloc_block(inst);
            if (!new_block_index) {
                rc = -2;
                goto exit;
            }
            ((uint32_t*)buffer)[d] = new_block_index;
            ext2_partition_write_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1, (char*)kheap_get_phys_address(buffer));
        }

        uint32_t saved_block = ((uint32_t*)buffer)[d];
        // Считать блок по адресу
        ext2_partition_read_block(inst, saved_block, 1, (char*)kheap_get_phys_address(buffer));

        if (!((uint32_t*)buffer)[f]) {
            // Адрес доп. блока не задан - выделяем блок и задаем адрес
            uint64_t new_block_index = ext2_alloc_block(inst);
            if (!new_block_index) {
                rc = -2;
                goto exit;
            }
            ((uint32_t*)buffer)[f] = new_block_index;
            ext2_partition_write_block(inst, saved_block, 1, (char*)kheap_get_phys_address(buffer));
        }

        saved_block = ((uint32_t*)buffer)[f];
        // Считать блок по адресу
        ext2_partition_read_block(inst, ((uint32_t*)buffer)[f], 1, (char*)kheap_get_phys_address(buffer));

        // Записать адрес блока в дополнительный блок
        ((uint32_t*)buffer)[g] = abs_block;

        // Записать измененный блок
        ext2_partition_write_block(inst, saved_block, 1, (char*)kheap_get_phys_address(buffer));

        rc = 1;
        goto exit;
    }

exit:

    kfree(buffer);

    return rc;
}

uint32_t ext2_read_inode_block(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_block, char* buffer)
{
    //Получить номер блока иноды внутри всего раздела
    uint32_t inode_block_abs = ext2_inode_block_absolute(inst, inode, inode_block);
    return ext2_partition_read_block(inst, inode_block_abs, 1, buffer);
}

uint32_t ext2_write_inode_block(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_num, uint32_t inode_block, char* buffer)
{
    // Если пытаемся записать в невыделенный номер блока
    // Расширить размер inode
    while (inode_block >= inode->num_blocks / (inst->block_size / 512)) {
		ext2_alloc_inode_block(inst, inode, inode_num, inode->num_blocks / (inst->block_size / 512));
		ext2_inode(inst, inode, inode_num);
	}

    //Получить номер блока иноды внутри всего раздела
    uint32_t inode_block_abs = ext2_inode_block_absolute(inst, inode, inode_block);
    return ext2_partition_write_block(inst, inode_block_abs, 1, buffer);
}

struct inode* ext2_mount(drive_partition_t* drive, struct superblock* sb)
{
    ext2_instance_t* instance = kmalloc(sizeof(ext2_instance_t));
    memset(instance, 0, sizeof(ext2_instance_t));

    instance->vfs_sb = sb;

    instance->partition = drive;
    instance->superblock = (ext2_superblock_t*)kmalloc(sizeof(ext2_superblock_t));
    instance->block_size = 1024;
    // Считать суперблок
    ext2_partition_read_block(instance, 1, 1, (char*)kheap_get_phys_address(instance->superblock));
    
    // Проверить магическую константу ext2
    if(instance->superblock->ext2_magic != EXT2_MAGIC) {
        kfree(instance->superblock);
        kfree(instance);
        return NULL;
    }

    if (instance->superblock->major >= 1) {
        // Ext2 версии 1 и новее, значит есть дополнительная часть superblock
        instance->file_size_64bit_flag = (instance->superblock->readonly_feature & EXT2_FEATURE_64BIT_FILE_SIZE) > 0;
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
    instance->bgd_start_block = (instance->block_size == 1024) ? 2 : 1;
    //Чтение BGD
    ext2_partition_read_block(instance, 
        instance->bgd_start_block,
        instance->bgds_blocks, 
        (char*)kheap_get_phys_address(instance->bgds));
    //Чтение корневой иноды с индексом 2
    ext2_inode_t* ext2_inode_root = new_ext2_inode();
    ext2_inode(instance, ext2_inode_root, 2);
    //Создать объект VFS корневой иноды файловой системы 
    struct inode* result = ext2_inode_to_vfs_inode(instance, ext2_inode_root, 2);

    kfree(ext2_inode_root);

    // Данные суперблока
    sb->fs_info = instance;
    sb->blocksize = instance->block_size;
    sb->operations = &sb_ops;

    return result;
}

void ext2_write_bgds(ext2_instance_t* inst)
{
    ext2_partition_write_block( inst, 
                                inst->bgd_start_block,
                                inst->bgds_blocks,
                                (char*)kheap_get_phys_address(inst->bgds));
}

void ext2_rewrite_superblock(ext2_instance_t* inst)
{
    ext2_partition_write_block( inst, 
                                1,
                                1,
                                (char*)kheap_get_phys_address(inst->superblock));
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
        ext2_partition_read_block(inst, bitmap_block_index, 1, (char*)kheap_get_phys_address(bitmap_buffer));

        for (uint32_t j = 0; j < inst->block_size / 4; j ++) {
            uint32_t bitmap = bitmap_buffer[j];

            for (int bit_i = 0; bit_i < 32; bit_i ++) {
                int is_bit_free = !((bitmap >> bit_i) & 1);

                if (is_bit_free) {
                    // Найден номер свободной inode
                    bitmap_buffer[j] |= (1U << bit_i);
                    // Запишем обновленную битмапу с занятым битом
                    ext2_partition_write_block(inst, bitmap_block_index, 1, (char*)kheap_get_phys_address(bitmap_buffer));
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

uint64_t ext2_alloc_block(ext2_instance_t* inst)
{
    uint8_t* buffer = (uint8_t*)kmalloc(inst->block_size);

    uint64_t bitmap_index = 0;
    uint64_t block_index = 0;
    uint64_t group_index = 0;

    for (uint64_t bgd_i = 0; bgd_i < inst->bgds_blocks; bgd_i ++) {
        if (inst->bgds[bgd_i].free_blocks > 0) {
            ext2_partition_read_block(inst, inst->bgds[bgd_i].block_bitmap, 1, (char*)kheap_get_phys_address(buffer));

            while (buffer[bitmap_index / 8] & (1 << (bitmap_index % 8))) {
                bitmap_index++;
            }

            group_index = bgd_i;
            block_index = bgd_i * inst->superblock->blocks_per_group + bitmap_index;
            break;
        }
    }

    if (block_index == 0) {
        // Не удалось найти свободный блок
        goto exit;
    }

    // Пометить блок в битмапе как занятый
    buffer[bitmap_index / 8] |= (1 << (bitmap_index % 8));
    ext2_partition_write_block(inst, inst->bgds[group_index].block_bitmap, 1, (uint8_t*)kheap_get_phys_address(buffer));

    // Уменьшить количество свободных блоков в группе и записать на диск
    inst->bgds[group_index].free_blocks--;
    ext2_write_bgds(inst);

    // Уменьшить количество свободных блоков в суперблоке и записать на диск
    inst->superblock->free_blocks--;
    ext2_rewrite_superblock(inst);

exit:
    kfree(buffer);
    return block_index;
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
    uint32_t blocks_count = (block_index + 1) * (inst->block_size / 512);
    if (inode->num_blocks < blocks_count) {
        inode->num_blocks = blocks_count;
    }

    // Перезаписать inode
    ext2_write_inode_metadata(inst, inode, node_num);

    return 0;
}

int ext2_create_dentry(ext2_instance_t* inst, struct inode* parent, const char* name, uint32_t inode, int type)
{
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

    ext2_read_inode_block(inst, e2_inode, block_index, (char*)kheap_get_phys_address(buffer));

    while (offset < e2_inode->size) {
        // dirent выравнены по размеру блока
        if (in_block_offset >= inst->block_size) {
            block_index++;
            in_block_offset -= inst->block_size;
            ext2_read_inode_block(inst, e2_inode, block_index, (char*)kheap_get_phys_address(buffer));
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
	ext2_write_inode_block(inst, e2_inode, parent->inode, block_index, (char*)kheap_get_phys_address(buffer));

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
    char* temp_buffer_phys = (char*)kheap_get_phys_address(temp_buffer);
    off_t current_offset = 0;

    for (off_t block_i = start_inode_block; block_i <= end_inode_block; block_i ++) {
        off_t left_offset = (block_i == start_inode_block) ? start_block_offset : 0;
        off_t right_offset = (block_i == end_inode_block) ? (end_size - 1) : (inst->block_size - 1);
        ext2_read_inode_block(inst, inode, block_i, temp_buffer_phys);

        memcpy(buf + current_offset, temp_buffer + left_offset, (right_offset - left_offset + 1));
        current_offset += (right_offset - left_offset + 1);
    }

    kfree(temp_buffer);
    return end_offset - offset;
}

// Записать данные файла inode
ssize_t write_inode_filedata(ext2_instance_t* inst, ext2_inode_t* inode, off_t offset, size_t size, const char * buf)
{
    //Не реализовано!!!

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

    return 0;
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
    ext2_partition_read_block(inst, inode_table_block + block_offset, 1, (char*)kheap_get_phys_address(buffer));
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
    ext2_partition_read_block(inst, inode_table_block + block_offset, 1, (char*)kheap_get_phys_address(buffer));
    // Заменить в нем область байт данной inode
    memcpy(buffer + offset_in_block * inst->superblock->inode_size, inode, sizeof(ext2_inode_t));
    // Записать измененный блок обратно на диск
    ext2_partition_write_block(inst, inode_table_block + block_offset, 1, (char*)kheap_get_phys_address(buffer));
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
    switch (ext2_dirent->type) {
        case EXT2_DT_REG:
            result->type = DT_REG;
            break;
        case EXT2_DT_LNK:
            result->type = DT_LNK;
            break;
        case EXT2_DT_DIR:
            result->type = DT_DIR;
            break;
    }

    return result;
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
    
    }
    if ((inode->mode & INODE_TYPE_MASK) == INODE_FLAG_BLOCKDEVICE) {
    
    }

    return result;
}

void ext2_open(struct inode* inode, uint32_t flags)
{

}

void ext2_close(struct inode* inode)
{
    
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

    ssize_t result = write_inode_filedata(inst, e2_inode, offset, count, buffer);
    file->pos += result;

    kfree(e2_inode);

    return result;
}

int ext2_truncate(struct inode* inode)
{
    ext2_instance_t* inst = (ext2_instance_t*)inode->sb->fs_info;
    ext2_inode_t* e2_inode = new_ext2_inode();
    ext2_inode(inst, e2_inode, inode->inode);
    e2_inode->size = 0;
    ext2_write_inode_metadata(inst, e2_inode, inode->inode);
    kfree(e2_inode);

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

int ext2_mkdir(struct inode* parent, const char* dir_name, uint32_t mode)
{
    ext2_instance_t* inst = (ext2_instance_t*)parent->sb->fs_info;
    
    if (ext2_find_dentry(parent->sb, parent->inode, dir_name) != WRONG_INODE_INDEX) {
        return -2; //уже существует
    }

    // Создать inode на диске
    uint32_t inode_num = ext2_alloc_inode(inst);
    // Прочитать inode
    ext2_inode_t *inode = new_ext2_inode();
    ext2_inode(inst, inode, inode_num);

    inode->mode = INODE_TYPE_DIRECTORY | (0xFFF & mode);
    inode->atime = 0;
    inode->ctime = 0;
    inode->mtime = 0;
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
    memset(block_temp, 0, sizeof(inst->block_size));
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
    ext2_write_inode_block(inst, inode, inode_num, 0, (char*)kheap_get_phys_address(block_temp));
    kfree(block_temp);

    // Увеличить количество ссылок родительской иноды
    ext2_inode(inst, inode, parent->inode);
    inode->hard_links++;
    ext2_write_inode_metadata(inst, inode, parent->inode);

    // Обновить информацию о свободных папках в bgds
    uint32_t new_inode_group = inode_num / inst->superblock->inodes_per_group;
	inst->bgds[new_inode_group].used_dirs++;
	ext2_write_bgds(inst);

    kfree(inode);
    return 0;
}

int ext2_mkfile(struct inode* parent, const char* file_name, uint32_t mode)
{
    ext2_instance_t* inst = (ext2_instance_t*)parent->sb->fs_info;

    if (ext2_find_dentry(parent->sb, parent->inode, file_name) != WRONG_INODE_INDEX) {
        return -2; //уже существует
    }

    // Создать inode на диске
    uint32_t inode_num = ext2_alloc_inode(inst);
    // Прочитать inode
    ext2_inode_t *inode = new_ext2_inode();
    ext2_inode(inst, inode, inode_num);

    inode->mode = INODE_TYPE_FILE | (0xFFF & mode);
    inode->atime = 0;
    inode->ctime = 0;
    inode->mtime = 0;
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

    ext2_write_bgds(inst);

    kfree(inode);
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
    char* buffer_phys = (char*)kheap_get_phys_address(buffer);
    //Прочитать начальный блок иноды
    ext2_read_inode_block(inst, parent_inode, block_offset, buffer_phys);
    //Проверка, не прочитан ли весь блок?
    while (curr_offset < parent_inode->size) {

        if (in_block_offset >= inst->block_size) {
            block_offset++;
            in_block_offset = 0;
            ext2_read_inode_block(inst, parent_inode, block_offset, buffer_phys);
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

uint64 ext2_find_dentry(struct superblock* sb, uint64_t parent_inode_index, const char *name)
{
    uint64_t result = WRONG_INODE_INDEX;
    ext2_instance_t* inst = (ext2_instance_t*)sb->fs_info;
    
    size_t name_len = strlen(name);

    //Получить родительскую иноду
    ext2_inode_t* parent_inode = new_ext2_inode();
    ext2_inode(inst, parent_inode, parent_inode_index);

    //Переменные
    uint32_t curr_offset = 0;
    uint32_t block_offset = 0;
    uint32_t in_block_offset = 0;
    
    //Выделить временную память под буфер блоков
    char* buffer = kmalloc(inst->block_size);
    char* buffer_phys = (char*)kheap_get_phys_address(buffer);
    
    //Прочитать начальный блок иноды
    ext2_read_inode_block(inst, parent_inode, block_offset, buffer_phys);
    //Пока не прочитаны все блоки
    while(curr_offset < parent_inode->size) {

        //Проверка, не прочитан ли весь блок?
        if(in_block_offset >= inst->block_size){
            block_offset++;
            in_block_offset = 0;
            ext2_read_inode_block(inst, parent_inode, block_offset, buffer_phys);
        }

        // Считанный dirent
        ext2_direntry_t* curr_entry = (ext2_direntry_t*)(buffer + in_block_offset);
        // Проверка совпадения имени
        if((curr_entry->inode != 0) && (strncmp(curr_entry->name, name, curr_entry->name_len) == 0) && name_len == curr_entry->name_len) {
            result = curr_entry->inode;
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