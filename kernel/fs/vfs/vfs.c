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

int vfs_mount(char* mount_path, drive_partition_t* partition)
{
    return vfs_mount_fs(mount_path, partition, "ext2"); //Захардкожено!!!
}

int vfs_mount_fs(char* mount_path, drive_partition_t* partition, char* fsname)
{
    struct dentry* mp = dentry_traverse_path(root_dentry, mount_path + 1);

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
        mp->sb = sb;
        mp->inode = root_inode->inode;
        sb->root_dir = mp;
        dentry_open(mp);
        
    } else {
        free_superblock(sb);
        return -4;  //Нет функции монтирования
    }

    vfs_mounts[mount_pos] = sb;

    return 0;
}

int vfs_unmount(char* mount_path)
{
    struct dentry* root_dentry = dentry_traverse_path(root_dentry, mount_path + 1);
    
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
    struct dentry* path_dentry = dentry_traverse_path(root_dentry, mount_path + 1);

    return path_dentry->sb;
}


struct superblock** vfs_get_mounts()
{
    return vfs_mounts;
}

struct inode* vfs_fopen(struct dentry* parent, const char* path, uint32_t flags, struct dentry** dentry)
{
    struct inode* result = NULL;
    int offset = path[0] == '/' ? 1 : 0;     // Пропускаем первый "/"
    
    // Корневая dentry
    struct dentry* curr_dentry = parent;
    if (!parent || path[0] == '/') {
        curr_dentry = root_dentry;
    }

    // Корневой superblock
    struct superblock* sb = root_dentry->sb;
    // Путь к файлу в ФС, отделенный от пути монтирования
    char* fs_path = (char*)path + offset;
    // Если обращаемся к корневой папке ФС - просто возращаем корень
    if(strlen(fs_path) == 0)
    {
        result = superblock_get_inode(sb, curr_dentry->inode);
        inode_open(result, flags);
        if (dentry) {
            dentry_open(curr_dentry);
            *dentry = curr_dentry;
        }
        return result;
    }

    //Временный буфер
    char* temp = (char*)kmalloc(strlen(fs_path) + 1);
    while (strlen(fs_path) > 0)
    {
        int len = 0;
        int is_nested = 0; // Есть ли вложение пути
        // Позиция / относительно начала
        char* slash_pos = strchr(fs_path, '/');
        
        if (slash_pos != NULL) { //Это директория
            len = slash_pos - fs_path;
            is_nested = 1;
        } else
            len = strlen(fs_path);
        
        strncpy(temp, fs_path, len);

        // Получить следующую dentry
        curr_dentry = superblock_get_dentry(sb, curr_dentry, temp);
        // Обновляем указатель на superblock
        sb = curr_dentry->sb;
        
        if (is_nested == 1) {
            if (curr_dentry == NULL) {
                // следующего файла или папки нет, выходим
                goto exit;
            }
        } else {
            if (curr_dentry != NULL) {
                // Попробуем найти ноду в списке суперблока
                result = superblock_get_inode(sb, curr_dentry->inode);
                // Увеличение счетчика, операции с ФС
                inode_open(result, flags);
                if (dentry) {
                    dentry_open(curr_dentry);
                    *dentry = curr_dentry;
                }
            }
            goto exit;
        }
        
        fs_path += len + 1;
    }
exit:
    kfree(temp);
    return result;
}