#include "vfs.h"
#include "file.h"
#include "string.h"
#include "filesystems.h"
#include "mem/kheap.h"
#include "superblock.h"

#define MAX_MOUNTS 100

struct superblock** vfs_mounts = NULL;
struct dentry*      root_dentry;

struct dirent* new_vfs_dirent()
{
    struct dirent* result = kmalloc(sizeof(struct dirent));
    memset(result, 0, sizeof(struct dirent));
    return result;
}

void vfs_init()
{
    vfs_mounts = (struct superblock**)kmalloc(sizeof(struct superblock*) * MAX_MOUNTS);
    memset(vfs_mounts, 0, sizeof(struct superblock*) * MAX_MOUNTS);

    // Создать корневой dentry с пустым именем
    root_dentry = new_dentry();
    root_dentry->inode = 0; // пока inode нет
    dentry_open(root_dentry); // Он должен всегда существовать
}

struct dentry* vfs_get_root_dentry()
{
    return root_dentry;
}

int vfs_get_free_mount_info_pos()
{
    for(int i = 0; i < MAX_MOUNTS; i ++){
        if(vfs_mounts[i] == NULL)
            return i;
    }

    return -1;
}

int vfs_mount_fs(const char* mount_path, drive_partition_t* partition, const char* fsname)
{
    struct dentry* mount_dent = vfs_dentry_traverse_path(root_dentry, mount_path);

    if (mount_dent == NULL) {
        // Путь монтирования отсутствует
        return -ERROR_NO_FILE;
    }

    // TODO: Проверить тип (должна быть директория)

    int mount_pos = vfs_get_free_mount_info_pos();
    if(mount_pos == -1)
        return -2;  //Не найдено место

    struct superblock* sb = new_superblock();
    sb->filesystem = filesystem_get_by_name(fsname); 
    sb->partition = partition;

    //вызов функции монтирования, если она определена
    if(sb->filesystem->mount) {
        struct inode* root_inode = sb->filesystem->mount(partition, sb);

        if(root_inode == NULL) {
            free_superblock(sb);
            return -5; //Ошибка при процессе монтирования
        }

        // Открыть корневую inode
        inode_open(root_inode, 0);

        // dentry монтирования
        mount_dent->sb = sb;
        mount_dent->inode = root_inode->inode;
        sb->root_dir = mount_dent;
        dentry_open(mount_dent);
        
    } else {
        free_superblock(sb);
        return -4;  //Нет функции монтирования
    }

    vfs_mounts[mount_pos] = sb;

    return 0;
}

int vfs_unmount(char* mount_path)
{
    struct dentry* root_dentry = vfs_dentry_traverse_path(root_dentry, mount_path);
    
    if (!root_dentry) {
        return -1;
    }

    // получение корневой dentry монтирования
    root_dentry = root_dentry->sb->root_dir;

    struct superblock* sb = root_dentry->sb;

    for (int i = 0; i < MAX_MOUNTS; i ++) {
        if (vfs_mounts[i] == sb) {
            //выполнить отмонтирование ФС
            if (sb->filesystem->unmount != NULL) {
                sb->filesystem->unmount(sb);
            }

            //Освободить память
            free_superblock(sb);
            vfs_mounts[i] = NULL;
            return 1;
        }
    }

    return -1;
}

struct superblock* vfs_get_mounted_partition(const char* mount_path)
{
    struct dentry* path_dentry = vfs_dentry_traverse_path(root_dentry, mount_path);

    return path_dentry->sb;
}


struct superblock** vfs_get_mounts()
{
    return vfs_mounts;
}

struct inode* vfs_fopen(struct dentry* parent, const char* path, struct dentry** dentry)
{
    struct dentry* result_dentry = vfs_dentry_traverse_path(parent, path);

    if (result_dentry) {

        struct inode* result = superblock_get_inode(result_dentry->sb, result_dentry->inode);

        if (result) {
            // Увеличение счетчика, операции с ФС
            inode_open(result, 0);

            if (dentry) {
                dentry_open(result_dentry);
                *dentry = result_dentry;
            }

            return result;
        }

        return NULL;
    }
    
    return NULL;
}

struct inode* vfs_fopen_parent(struct dentry* child)
{
    struct inode* parent_inode = superblock_get_inode(child->sb, child->parent->inode);

    if (parent_inode) {
        inode_open(parent_inode, 0);
    }

    return parent_inode;
}

struct dentry* vfs_dentry_traverse_path(struct dentry* parent, const char* path)
{
    if (path == NULL) {
        return parent;
    }

    if (path[0] == '\0') {
        // Пустая строка пути - возвращаем родителя
        return parent;
    }

    if (path[0] == '/') {
        // Путь абсолюютный - ищем относительно корня
        parent = root_dentry;
        path++;
    } else if (parent == NULL) {
        // Путь не абсолютный и родитель не указан - нечего искать
        return NULL;
    }

    return dentry_traverse_path(parent, path);
}

void vfs_dentry_get_absolute_path(struct dentry* p_dentry, size_t* p_required_size, char* p_result)
{
    if (p_dentry == root_dentry) {
        // Это корневая dentry - пишем / и выходим
        if (p_required_size) {
            *p_required_size = 1;
        }

        if (p_result) {
            strcpy(p_result, "/");
        }

        return;
    }

    if (p_result) {
        // Обнуление длины строки, так как функция dentry_get_absolute_path записывает конкатенацией
        p_result[0] = '\0';
    }

    dentry_get_absolute_path(p_dentry, p_required_size, p_result);
}

int vfs_is_path_absolute(const char* path)
{
    return path != NULL && path[0] == '/';
}