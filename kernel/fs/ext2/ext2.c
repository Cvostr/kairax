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

int ext2_partition_read_block(ext2_instance_t* inst, uint64_t block_start, uint64_t blocks, char* buffer){
    uint64_t    start_lba = block_start * inst->block_size / 512;
    uint64_t    lba_count = blocks  * inst->block_size / 512;
    partition_read(inst->partition, start_lba, lba_count, buffer);
}

vfs_inode_t* ext2_mount(drive_partition_t* drive){
    ext2_instance_t* instance = kmalloc(sizeof(ext2_instance_t));
    memset(instance, 0, sizeof(ext2_instance_t));

    instance->partition = drive;
    instance->superblock = kmalloc(sizeof(ext2_superblock_t));
    partition_read(drive, 2, 2, kheap_get_phys_address(instance->superblock));
    
    printf("%s %i %i\n", instance->superblock->vol_name, instance->superblock->ext2_magic, instance->block_size);
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
    uint64_t bgd_start_block = (instance->block_size == 1024) ? bgd_start_block = 2 : 1;
    //Чтение BGD
    ext2_partition_read_block(instance, bgd_start_block, instance->bgds_blocks, kheap_get_phys_address(instance->bgds));
    //Чтение корневой иноды с индексом 2
    ext2_inode_t* ext2_inode_root = kmalloc(sizeof(ext2_inode_t));
    ext2_inode(instance, ext2_inode_root, 2);

    vfs_inode_t* result = new_vfs_inode();
    result->inode = 2;
    result->mask = ext2_inode_root->permission;
    result->fs_d = instance;
    result->flags = VFS_FLAG_DIRECTORY;

    result->create_time = ext2_inode_root->ctime;
    result->access_time = ext2_inode_root->atime;
    result->modify_time = ext2_inode_root->mtime;

    result->operations.chmod = ext2_chmod;
    result->operations.mkdir = ext2_mkdir;
    result->operations.mkfile = ext2_mkfile;

    return result;
}

void ext2_inode(ext2_instance_t* inst, ext2_inode_t* inode, uint64_t node_index){
    //Группа, к которой принадлежит инода с указанным индексом
    uint32_t inode_group = node_index / inst->superblock->inodes_per_group;
    //Блок
    uint32_t inode_table_block = inst->bgds[inode_group].inode_table;

    uint32_t idx_in_group = node_index - inode_group * inst->superblock->inodes_per_group;

    uint32_t block_offset = (idx_in_group - 1) * inst->superblock->inode_size / inst->block_size;

    uint32_t offset_in_block = (idx_in_group - 1) - block_offset * (inst->block_size / inst->superblock->inode_size);

    char* buffer = kmalloc(inst->block_size);
    ext2_partition_read_block(inst, inode_table_block + block_offset, 1, V2P(buffer));
    memcpy(inode, buffer, sizeof(ext2_inode_t));
    kfree(buffer);
}

uint32_t ext2_read(vfs_inode_t* file, uint32_t offset, uint32_t size, char* buffer){

}

void ext2_chmod(vfs_inode_t * file, uint32_t mode){

}

void ext2_mkdir(vfs_inode_t* parent, char* dir_name){

}

void ext2_mkfile(vfs_inode_t* parent, char* file_name){

}