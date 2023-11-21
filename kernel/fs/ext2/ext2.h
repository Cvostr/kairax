#ifndef _EXT2_H
#define _EXT2_H

#include "types.h"
#include "../vfs/vfs.h"
#include "../vfs/file.h"
#include "../vfs/superblock.h"
#include "../vfs/dentry.h"

#define EXT2_DIRECT_BLOCKS 12

#define EXT2_FEATURE_64BIT_FILE_SIZE 0x0002

#define EXT2_MAGIC 61267

typedef struct PACKED {
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t su_blocks;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t superblock_idx;
    uint32_t log2block_size;
    uint32_t log2frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;

    uint32_t mtime;
    uint32_t wtime;

    uint16_t mount_count;
    uint16_t mount_allowed_count;
    uint16_t ext2_magic;
    uint16_t fs_state;
    uint16_t err;
    uint16_t minor;

    uint32_t last_check;
    uint32_t interval;
    uint32_t os_id;
    uint32_t major;

    uint16_t r_userid;
    uint16_t r_groupid;
    //Расширенная часть для версии >= 1
    uint32_t first_inode;
    uint16_t inode_size;
    uint16_t superblock_group;
    uint32_t optional_feature;
    uint32_t required_feature;
    uint32_t readonly_feature;
    char fs_id[16];
    char vol_name[16];
    char last_mount_path[64];
    uint32_t compression_method;
    uint8_t file_pre_alloc_blocks;
    uint8_t dir_pre_alloc_blocks;
    uint16_t unused1;
    char journal_id[16];
    uint32_t journal_inode;
    uint32_t journal_device;
    uint32_t orphan_head;

    char unused2[1024-236];
} ext2_superblock_t;

typedef struct PACKED {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks;
    uint16_t free_inodes;
    uint16_t used_dirs;
    char unused[14];
} ext2_bgd_t;


typedef struct PACKED {
    uint32_t    inode;
    uint16_t    size;
    uint8_t     name_len;
    uint8_t     type;
    char        name[];
} ext2_direntry_t;

typedef struct PACKED {
    uint16_t mode;
    uint16_t userid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t hard_links;
    uint32_t num_blocks; //LBA блоки по 512 байт
    uint32_t flags;
    uint32_t os_specific1;
    uint32_t blocks[EXT2_DIRECT_BLOCKS + 3];
    uint32_t generation;
    uint32_t file_acl;
    union {
        uint32_t dir_acl;   // для директорий
        uint32_t size_high;
    };
    uint32_t faddr;
    char os_specific2[12];
} ext2_inode_t;

typedef struct PACKED {
    drive_partition_t*  partition;
    ext2_superblock_t*  superblock;
    struct superblock*  vfs_sb;
    ext2_bgd_t*         bgds;

    size_t              block_size;
    uint64_t            total_groups;

    uint64_t            bgd_start_block;
    uint64_t            bgds_blocks;

    int                 file_size_64bit_flag;
} ext2_instance_t;

#define EXT2_DT_UNKNOWN  0
#define EXT2_DT_FIFO     5
#define EXT2_DT_CHR      3
#define EXT2_DT_DIR      2
#define EXT2_DT_BLK      4
#define EXT2_DT_REG      1      // Обычный файл
#define EXT2_DT_LNK      7
#define EXT2_DT_SOCK     6

void ext2_init();

ext2_inode_t* new_ext2_inode();

size_t ext2_inode_get_size(ext2_instance_t* inst, ext2_inode_t* inode);

void ext2_inode_set_size(ext2_instance_t* inst, ext2_inode_t* inode, size_t size);

uint32_t ext2_partition_read_block(ext2_instance_t* inst, uint64_t block_start, uint64_t blocks, char* buffer);

uint32_t ext2_partition_write_block(ext2_instance_t* inst, uint64_t block_start, uint64_t blocks, char* buffer);

uint32_t ext2_inode_block_absolute(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_block_index);

uint32_t ext2_read_inode_block(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_block, char* buffer);

uint32_t ext2_write_inode_block(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_num, uint32_t inode_block, char* buffer);

// Добавление блока с номером abs_block к ноде
int ext2_inode_add_block(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_index, uint32_t abs_block, uint32_t inode_block);

// Вызывается VFS при монтировании
struct inode* ext2_mount(drive_partition_t* drive, struct superblock* sb);

// Прочитать данные файла inode
ssize_t read_inode_filedata(ext2_instance_t* inst, ext2_inode_t* inode, off_t offset, size_t size, char * buf);

// Записать данные файла inode
ssize_t write_inode_filedata(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t inode_num, off_t offset, size_t size, const char * buf);

// Перезаписать на диск таблицу блоков
void ext2_write_bgds(ext2_instance_t* inst);

// Перезаписать на диск суперблок
void ext2_rewrite_superblock(ext2_instance_t* inst);

// Получение inode по номеру
void ext2_inode(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t node_index);

// Запись метаданных inode
void ext2_write_inode_metadata(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t node_index);

// Создание новой inode на диске
uint32_t ext2_alloc_inode(ext2_instance_t* inst);

// Выделение нового блока на диске
uint64_t ext2_alloc_block(ext2_instance_t* inst);

// Выделить блок для иноды
int ext2_alloc_inode_block(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t node_num, uint32_t block_index);

// Создать direntry в иноде на другую inode с указанным типом
int ext2_create_dentry(ext2_instance_t* inst, struct inode* parent, const char* name, uint32_t inode, int type);

// Удалить direntry в иноде по указанному имени
int ext2_remove_dentry(ext2_instance_t* inst, struct inode* parent, const char* name, int* orig_type);

// Преобразовать inode ФС ext2 в общий для ядра
struct inode* ext2_inode_to_vfs_inode(ext2_instance_t* inst, ext2_inode_t* inode, uint32_t ino_num);

// Преобразовать dirent ФС ext2 в общий для ядра
struct dirent* ext2_dirent_to_vfs_dirent(ext2_direntry_t* ext2_dirent);

// --------------- Функции inode и file -----------

int ext2_mkdir(struct inode* parent, const char* dir_name, uint32_t mode);

int ext2_mkfile(struct inode* parent, const char* file_name, uint32_t mode);

int ext2_chmod(struct inode * inode, uint32_t mode);

int ext2_truncate(struct inode* inode);

int ext2_rename(struct inode* oldparent, struct dentry* orig_dentry, struct inode* newparent, const char* newname);

struct inode* ext2_read_node(struct superblock* sb, uint64_t ino_num);

uint64 ext2_find_dentry(struct superblock* sb, uint64_t parent_inode_index, const char *name);

struct dirent* ext2_readdir(struct inode* dir, uint32_t index);

struct dirent* ext2_file_readdir(struct file* dir, uint32_t index);

ssize_t ext2_file_read(struct file* file, char* buffer, size_t count, loff_t offset);

ssize_t ext2_file_write(struct file* file, const char* buffer, size_t count, loff_t offset);

// -----------------------------------------------

#endif