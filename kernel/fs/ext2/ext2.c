#include "ext2.h"
#include "../vfs/filesystems.h"
#include "string.h"
#include "memory/hh_offset.h"

void ext2_init(){
    filesystem_t* ext2fs = new_filesystem();
    ext2fs->name = "ext2";
    ext2fs->mount = ext2_mount;

    filesystem_register(ext2fs);
}

ext2_inode_t* new_ext2_inode(){
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

uint32_t ext2_inode_block_absolute(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_block_index){
    unsigned int p = inst->block_size / 4;
    int a = inode_block_index - EXT2_DIRECT_BLOCKS;
    int d, e, f, g;
    if(a < 0){
        return inode->blocks[inode_block_index];
    }
    //Выделение временной памяти под считываемые блоки
    unsigned int* tmp = kmalloc(inst->block_size);
    //Получение физического адреса выделенной памяти
    unsigned int* tmp_phys = kheap_get_phys_address(tmp);
    int b = a - p;
    if(b < 0){
        ext2_partition_read_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS], 1, tmp_phys);
        uint32_t result = tmp[a];
        kfree(tmp);
        return result;
    }
    int c = b - p * p;
    if(c < 0){
        c = b / p;
        d = b - c * p;
        ext2_partition_read_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 1], 1, tmp_phys);
        ext2_partition_read_block(inst, tmp[c], 1, tmp_phys);
        uint32_t result = tmp[d];
        kfree(tmp);
        return result;
    }
    d = c - p * p * p;
    if(d < 0){
        e = c / (p * p);
        f = (c - e * p * p) / p;
        g = (c - e * p * p - f * p);
        ext2_partition_read_block(inst, inode->blocks[EXT2_DIRECT_BLOCKS + 2], 1, tmp_phys);
        ext2_partition_read_block(inst, tmp[e], 1, tmp_phys);
        ext2_partition_read_block(inst, tmp[f], 1, tmp_phys);
        uint32_t result = tmp[g];
        kfree(tmp);
        return result;
    }
    return 0;   
}

uint32_t ext2_read_inode_block(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_block, char* buffer){
    //Получить номер блока иноды внутри всего раздела
    uint32_t inode_block_abs = ext2_inode_block_absolute(inst, inode, inode_block);
    return ext2_partition_read_block(inst, inode_block_abs, 1, buffer);
}

vfs_inode_t* ext2_mount(drive_partition_t* drive){
    ext2_instance_t* instance = kmalloc(sizeof(ext2_instance_t));
    memset(instance, 0, sizeof(ext2_instance_t));

    instance->partition = drive;
    instance->superblock = kmalloc(sizeof(ext2_superblock_t));
    instance->block_size = 1024;
    ext2_partition_read_block(instance, 1, 1, kheap_get_phys_address(instance->superblock));
    
    //Сохранить некоторые полезные значения
    instance->block_size = (1024 << instance->superblock->log2block_size);
    instance->total_groups = instance->superblock->total_blocks / instance->superblock->blocks_per_group;
    if(instance->superblock->blocks_per_group * instance->total_groups < instance->superblock->total_blocks)
        instance->total_groups++;
    //необходимое количество блоков дескрипторов групп (BGD)
    instance->bgds_blocks = (instance->total_groups * sizeof(ext2_bgd_t)) / instance->block_size;
    if(instance->bgds_blocks * instance->block_size < instance->total_groups * sizeof(ext2_bgd_t))
        instance->bgds_blocks++;

    instance->bgds = kmalloc(sizeof(ext2_bgd_t) * instance->bgds_blocks * instance->block_size);
    uint64_t bgd_start_block = (instance->block_size == 1024) ? 2 : 1;
    //Чтение BGD
    ext2_partition_read_block(instance, bgd_start_block, instance->bgds_blocks, kheap_get_phys_address(instance->bgds));
    //Чтение корневой иноды с индексом 2
    ext2_inode_t* ext2_inode_root = new_ext2_inode();
    ext2_inode(instance, ext2_inode_root, 2);
    instance->root_inode = ext2_inode_root;

    printf("%s %i %i\n", instance->superblock->vol_name, instance->superblock->ext2_magic, instance->block_size);
    //Создать объект VFS корневой иноды файловой системы 
    vfs_inode_t* result = new_vfs_inode();
    strcpy(result->name, "/");
    result->inode = 2;              //2 - стандартный индекс корневой иноды
    result->mask = ext2_inode_root->permission;
    result->fs_d = instance;        //Сохранение указателя на информацию о файловой системе
    result->flags = VFS_FLAG_DIRECTORY;

    result->create_time = ext2_inode_root->ctime;
    result->access_time = ext2_inode_root->atime;
    result->modify_time = ext2_inode_root->mtime;

    result->operations.open = ext2_open;
    result->operations.chmod = ext2_chmod;
    result->operations.mkdir = ext2_mkdir;
    result->operations.mkfile = ext2_mkfile;
    result->operations.finddir = ext2_finddir;
    result->operations.readdir = ext2_readdir;

    kfree(ext2_inode_root);

    return result;
}

void ext2_inode(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t node_index){
    //Группа, к которой принадлежит инода с указанным индексом
    uint32_t inode_group = node_index / inst->superblock->inodes_per_group;
    //Блок
    uint32_t inode_table_block = inst->bgds[inode_group].inode_table;

    uint32_t idx_in_group = node_index - inode_group * inst->superblock->inodes_per_group;

    uint32_t block_offset = (idx_in_group - 1) * inst->superblock->inode_size / inst->block_size;

    uint32_t offset_in_block = (idx_in_group - 1) - block_offset * (inst->block_size / inst->superblock->inode_size);

    //printf("INODE GROUP %i, TB %i, OFFSET %i\n", inode_group, inode_table_block, offset_in_block);

    char* buffer = kmalloc(inst->block_size);
    ext2_partition_read_block(inst, inode_table_block + block_offset, 1, kheap_get_phys_address(buffer));
    memcpy(inode, buffer + offset_in_block * inst->superblock->inode_size, sizeof(ext2_inode_t));

    kfree(buffer);
}

vfs_inode_t* ext2_inode_to_vfs_inode(ext2_instance_t* inst, ext2_inode_t* inode, ext2_direntry_t* dirent){
    vfs_inode_t* result = new_vfs_inode();
    result->inode = dirent->inode;
    result->fs_d = inst;
    strncpy(result->name, dirent->name, dirent->name_len);
    result->uid = inode->userid;
    result->gid = inode->gid;
    result->size = inode->size;
    result->mask = inode->permission & 0xFFF;
    result->access_time = inode->atime;
    result->create_time = inode->ctime;
    result->modify_time = inode->mtime;

    //result->
    result->flags = 0;
    result->operations.chmod = ext2_chmod;
    result->operations.open = ext2_open;
    //result->operations.close = ext2_close;

    if ((inode->permission & EXT2_INODE_FILE) == EXT2_INODE_FILE) {
        result->flags |= VFS_FLAG_FILE;
        result->operations.read = ext2_read;
        result->operations.write = ext2_write;
    }
    if((inode->permission & EXT2_INODE_DIR) == EXT2_INODE_DIR){
        result->flags |= VFS_FLAG_DIRECTORY;
        result->operations.mkdir = ext2_mkdir;
        result->operations.mkfile = ext2_mkfile;
        result->operations.finddir = ext2_finddir;
        result->operations.readdir = ext2_readdir;
    }
    if((inode->permission & EXT2_INODE_LINK) == EXT2_INODE_LINK){
        result->flags |= VFS_FLAG_SYMLINK;
    }

    return result;
}

void ext2_open(vfs_inode_t* inode, uint32_t flags){
    printf("OPENING NODE %i NAME %s\n", inode->inode, inode->name);
}

uint32_t ext2_read(vfs_inode_t* file, uint32_t offset, uint32_t size, char* buffer){

}

uint32_t ext2_write(vfs_inode_t* file, uint32_t offset, uint32_t size, char* buffer){

}

void ext2_chmod(vfs_inode_t * file, uint32_t mode){

}

void ext2_mkdir(vfs_inode_t* parent, char* dir_name){

}

void ext2_mkfile(vfs_inode_t* parent, char* file_name){

}

dirent_t* ext2_readdir(vfs_inode_t* dir, uint32_t index){
    ext2_instance_t* inst = (ext2_instance_t*)dir->fs_d;
    //Получить родительскую иноду
    ext2_inode_t* parent_inode = new_ext2_inode();
    ext2_inode(inst, parent_inode, dir->inode);
}

vfs_inode_t* ext2_finddir(vfs_inode_t * parent, char *name){
    ext2_instance_t* inst = (ext2_instance_t*)parent->fs_d;
    //Получить родительскую иноду
    ext2_inode_t* parent_inode = new_ext2_inode();
    ext2_inode(inst, parent_inode, parent->inode);
    //Переменные
    uint32_t curr_offset = 0;
    uint32_t block_offset = 0;
    uint32_t in_block_offset = 0;
    //Выделить временную память под буфер блоков
    char* buffer = kmalloc(inst->block_size);
    char* buffer_phys = kheap_get_phys_address(buffer);

    ext2_read_inode_block(inst, parent_inode, block_offset, buffer_phys);
    while(curr_offset < parent_inode->size) {
        //Проверка, не прочитан ли весь блок?
        if(in_block_offset >= inst->block_size){
            block_offset++;
            in_block_offset = 0;
            ext2_read_inode_block(inst, parent_inode, block_offset, buffer_phys);
        }

        ext2_direntry_t* curr_entry = (ext2_direntry_t*)(buffer + in_block_offset);
        if(curr_entry->inode != 0){
            if(strncmp(curr_entry->name, name, curr_entry->name_len) == 0){
                //printf("FOUND DIRENT ID %i, NAME %s\n", curr_entry->inode, curr_entry->name, curr_entry->name_len);
                ext2_inode_t* inode = new_ext2_inode();
                ext2_inode(inst, inode, curr_entry->inode);
                vfs_inode_t* result = ext2_inode_to_vfs_inode(inst, inode, curr_entry);
                kfree(buffer);
                return result;
            }
        }

        in_block_offset += curr_entry->size;
        curr_offset += curr_entry->size;
    }
    return NULL;
}