#ifndef _PROCFS_H
#define _PROCFS_H

#include "types.h"
#include "../vfs/vfs.h"
#include "../vfs/file.h"
#include "../vfs/superblock.h"

void procfs_init();

struct inode* procfs_mount(drive_partition_t* drive, struct superblock* sb);
int procfs_unmount(struct superblock* sb);

// Функция для чтения корневой директории.
// Выводит все директории процессов
struct dirent* procfs_root_file_readdir(struct file* dir, uint32_t index);
// Функция для чтения директории процесса
struct dirent* procfs_process_file_readdir(struct file* dir, uint32_t index);

uint64_t procfs_find_dentry(struct superblock* sb, struct inode* vfs_parent_inode, const char *name, int* type);
struct inode* procfs_read_node(struct superblock* sb, ino_t ino_num);

// для процесса
ssize_t procfs_status_read(struct file* file, char* buffer, size_t count, loff_t offset);
ssize_t procfs_cmdline_read(struct file* file, char* buffer, size_t count, loff_t offset);
ssize_t procfs_maps_read(struct file* file, char* buffer, size_t count, loff_t offset);
ssize_t procfs_cwd_readlink(struct inode* inode, char* pathbuf, size_t pathbuflen);

// для файлов корня
ssize_t procfs_mounts_read(struct file* file, char* buffer, size_t count, loff_t offset);
ssize_t procfs_modules_read(struct file* file, char* buffer, size_t count, loff_t offset);

#endif
