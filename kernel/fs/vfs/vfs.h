#ifndef _VFS_H
#define _VFS_H

#include "kairax/types.h"
#include "drivers/storage/partitions/storage_partitions.h"
#include "filesystems.h"

struct dirent* new_vfs_dirent();

struct dentry* get_mount_dentry(const char* mount_path);

void vfs_init();

int vfs_mount_fs(const char* mount_path, drive_partition_t* partition, const char* fsname);

int vfs_unmount(const char *mount_path);

struct superblock* vfs_get_mounted_partition(const char* mount_path);

struct superblock* vfs_get_mounted_sb(int index);

//Функции файловой системы

struct inode* vfs_fopen(struct dentry* parent, const char* path, struct dentry** dentry);
/// @brief Найти dentry и inode файла по пути относительно parent. При успехе увеличивает счетчик ссылок dentry и inode
/// @param parent dentry, относительно которого ищем, если указан относительный путь. При NULL используется корень файловой системы
/// @param path строка пути
/// @param[out] dentry dentry найденного файла 
/// @param[out] inode inode найденного файла
/// @param flags флаги поиска
/// @return код ошибки, при успехе 0
int vfs_fopen_ex(struct dentry* parent, const char* path, struct dentry** dentry, struct inode** inode, int flags);

struct inode* vfs_fopen_parent(struct dentry* child);

struct dentry* vfs_get_root_dentry();

struct dentry* vfs_dentry_traverse_path(struct dentry* parent, const char* path, int flags);
int vfs_dentry_traverse_path_ex(struct dentry* parent, const char* path, int flags, int depth, struct dentry **out);

void vfs_dentry_get_absolute_path(struct dentry* p_dentry, size_t* p_required_size, char* p_result);

int vfs_is_path_absolute(const char* path);

#endif