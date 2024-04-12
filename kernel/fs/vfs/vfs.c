#include "vfs.h"
#include "file.h"
#include "string.h"
#include "filesystems.h"
#include "mem/kheap.h"
#include "superblock.h"
#include "list/list.h"

list_t* vfs_mounts = NULL;
struct dentry*      root_dentry;

struct dirent* new_vfs_dirent()
{
    struct dirent* result = kmalloc(sizeof(struct dirent));
    memset(result, 0, sizeof(struct dirent));
    return result;
}

void vfs_init()
{
    vfs_mounts = create_list();

    // Создать корневой dentry с пустым именем
    root_dentry = new_dentry();
    root_dentry->d_inode = NULL; // пока inode нет
    root_dentry->flags |= DENTRY_TYPE_DIRECTORY;
    dentry_open(root_dentry); // Он должен всегда существовать
}

struct dentry* vfs_get_root_dentry()
{
    return root_dentry;
}

void dentry_debug_tree()
{
    dentry_debug_tree_entry(root_dentry, 0);
}

int vfs_mount_fs(const char* mount_path, drive_partition_t* partition, const char* fsname)
{
    struct dentry* mount_dent = vfs_dentry_traverse_path(root_dentry, mount_path);

    if (mount_dent == NULL) {
        // Путь монтирования отсутствует
        return -ERROR_NO_FILE;
    }

    // Проверить тип (должна быть директория)
    if ((mount_dent->flags & DENTRY_TYPE_DIRECTORY) == 0) {
        return -ERROR_NOT_A_DIRECTORY;
    }

    if ((mount_dent->flags & DENTRY_MOUNTPOINT) == DENTRY_MOUNTPOINT) {
        return -ERROR_BUSY;
    }

    struct superblock* sb = new_superblock();
    sb->filesystem = filesystem_get_by_name(fsname); 
    sb->partition = partition;

    //вызов функции монтирования, если она определена
    if(sb->filesystem->mount) {
        struct inode* root_inode = sb->filesystem->mount(partition, sb);

        if(root_inode == NULL) {
            free_superblock(sb);
            //Ошибка при процессе монтирования, считаем как поврежденный superblock
            return -ERROR_INVALID_VALUE;
        }

        // Открыть корневую inode
        inode_open(root_inode, 0);

        // dentry монтирования
        mount_dent->sb = sb;
        mount_dent->d_inode = root_inode;
        //atomic_inc(&mount_dent->d_inode->reference_count);
        mount_dent->flags |= DENTRY_MOUNTPOINT;
        // Сохранение указателя на dentry монтирования в superblock с увеличением счетчика
        sb->root_dir = mount_dent;
        dentry_open(mount_dent);
        
    } else {
        free_superblock(sb);
        return -ERROR_INVALID_VALUE;  //Нет функции монтирования
    }

    // Сохранить в списке
    list_add(vfs_mounts, sb);

    return 0;
}

int vfs_unmount(char* mount_path)
{
    struct dentry* root_dentry = vfs_dentry_traverse_path(root_dentry, mount_path);
    
    if (!root_dentry) {
        return -ERROR_INVALID_VALUE;
    }

    // получение корневой dentry монтирования
    root_dentry = root_dentry->sb->root_dir;

    struct superblock* sb = root_dentry->sb;

    struct list_node* sb_list_node = list_get_node(vfs_mounts, sb);

    if (sb_list_node) {
        //выполнить отмонтирование ФС
        if (sb->filesystem->unmount != NULL) {
            sb->filesystem->unmount(sb);
        }

        //Освободить память
        free_superblock(sb);

        // Удалить из списка
        list_remove(vfs_mounts, sb);
    } else {
        return -ERROR_INVALID_VALUE;
    }

    return 0;
}

struct superblock* vfs_get_mounted_partition(const char* mount_path)
{
    struct dentry* path_dentry = vfs_dentry_traverse_path(root_dentry, mount_path);

    return path_dentry->sb;
}


struct superblock* vfs_get_mounted_sb(int index)
{
    return list_get(vfs_mounts, index);
}

struct inode* vfs_fopen(struct dentry* parent, const char* path, struct dentry** dentry)
{
    struct dentry* result_dentry = vfs_dentry_traverse_path(parent, path);

    if (result_dentry) {

        struct inode* result = superblock_get_inode(result_dentry->sb, result_dentry->d_inode->inode);

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
    if (child->parent == NULL) {
        return NULL;
    }

    struct inode* parent_inode = superblock_get_inode(child->sb, child->parent->d_inode->inode);

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