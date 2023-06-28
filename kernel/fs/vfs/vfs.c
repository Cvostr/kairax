#include "vfs.h"
#include "file.h"
#include "string.h"
#include "filesystems.h"
#include "mem/kheap.h"

#define MAX_MOUNTS 100

vfs_mount_info_t** vfs_mounts = NULL;

struct dirent* new_vfs_dirent()
{
    struct dirent* result = kmalloc(sizeof(struct dirent));
    memset(result, 0, sizeof(struct dirent));
    return result;
}

void vfs_init()
{
    vfs_mounts = (vfs_mount_info_t**)kmalloc(sizeof(vfs_mount_info_t*) * MAX_MOUNTS);
    memset(vfs_mounts, 0, sizeof(vfs_mount_info_t*) * MAX_MOUNTS);
}

vfs_mount_info_t* new_vfs_mount_info()
{
    vfs_mount_info_t* result = (vfs_mount_info_t*)kmalloc(sizeof(vfs_mount_info_t));
    memset(result, 0, sizeof(vfs_mount_info_t));
    return result;
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
    if(vfs_get_mounted_partition(mount_path) != NULL){
        return -1;  //Данный путь уже используется
    }
    int mount_pos = vfs_get_free_mount_info_pos();
    if(mount_pos == -1)
        return -2;  //Не найдено место

    vfs_mount_info_t* mount_info = new_vfs_mount_info();
    mount_info->partition = partition;
    mount_info->filesystem = filesystem_get_by_name(fsname); 
    strcpy(mount_info->mount_path, mount_path);
    //вызов функции монтирования, если она определена
    if(mount_info->filesystem->mount) {
        mount_info->root_node = mount_info->filesystem->mount(partition);
        atomic_inc(&mount_info->root_node->reference_count);
    } else
        return -4;  //Нет функции монтирования

    if(mount_info->root_node == NULL)
        return -5; //Ошибка при процессе монтирования

    //Удалить / в конце, если есть
    int path_len = strlen(mount_info->mount_path);
    if(mount_info->mount_path[path_len - 1] == '/')
        mount_info->mount_path[path_len - 1] = '\0';

    vfs_mounts[mount_pos] = mount_info;
}

int vfs_unmount(char* mount_path)
{
    for(int i = 0; i < MAX_MOUNTS; i ++) {

        if(strcmp(vfs_mounts[i]->mount_path, mount_path) == 0) {

            vfs_mount_info_t* mount_info = vfs_mounts[i];
            //выполнить отмонтирование ФС
            if (mount_info->filesystem->unmount != NULL) {
                mount_info->filesystem->unmount(mount_info->partition);
            }

            //Освободить память
            kfree(vfs_mounts[i]);
            vfs_mounts[i] = NULL;
            return 1;
        }
    }

    return -1;//Данный путь не использовался
}

vfs_mount_info_t* vfs_get_mounted_partition(const char* mount_path)
{
    for (int i = 0; i < MAX_MOUNTS; i ++) {
        
        if(vfs_mounts[i] == NULL)
            continue;
        
        if(strcmp(vfs_mounts[i]->mount_path, mount_path) == 0){
            return vfs_mounts[i];
        }
    }
    return NULL;
}

void path_to_slash(char* path)
{
    int i = strlen(path);
    i--;
    while(i >= 0){
        char ch = path[i];
        if(ch != '/'){
            path[i] = '\0';
            i--;
        }else{
            path[i] = '\0';
            break;
        }
    }
}

vfs_mount_info_t* vfs_get_mounted_partition_split(const char* path, int* offset)
{
    int path_len = strlen(path);
    char* temp = kmalloc(path_len + 1);
    strcpy(temp, path);

    vfs_mount_info_t* result = NULL;
    while (result == NULL) {
        result = vfs_get_mounted_partition(temp);

        if (result == NULL){
            if (strlen(temp) == 0)
                return NULL;
            path_to_slash(temp);
        }
    }

    *offset = strlen(temp);

    kfree(temp);

    return result;
}

vfs_mount_info_t** vfs_get_mounts()
{
    return vfs_mounts;
}

struct inode* vfs_fopen(const char* path, uint32_t flags)
{
    int offset = 0;
    // Найти смонтированную файловую систему по пути, в offset - смещение пути монтирования
    vfs_mount_info_t* mount_info = vfs_get_mounted_partition_split(path, &offset);
    //Корневой узел найденной ФС
    struct inode* curr_node = mount_info->root_node;

    offset += 1;
    // Путь к файлу в ФС, отделенный от пути монтирования
    char* fs_path = (char*)path + offset;

    // Если обращаемся к корневой папке ФС - просто возращаем корень
    if(strlen(fs_path) == 0)
    {
        atomic_inc(&curr_node->reference_count);
        return curr_node;
    }

    //Временный буфер
    char* temp = (char*)kmalloc(strlen(fs_path) + 1);

    while(strlen(fs_path) > 0)
    {
        int len = 0;
        int is_dir = 0;
        // Позиция / относительно начала
        char* slash_pos = strchr(fs_path, '/');

        if (slash_pos != NULL) { //Это директория
            len = slash_pos - fs_path;
            is_dir = 1;
        } else 
            len = strlen(fs_path);

        strncpy(temp, fs_path, len);

        struct inode* next = vfs_finddir(curr_node, temp);
        if(is_dir == 1) {
            if(next != NULL) {
                // Освободить память текущей ноды
                if(curr_node != mount_info->root_node)
                    kfree(curr_node);

                // Заменить указатель
                curr_node = next;
            }else {
                kfree(temp);
                return NULL;
            }
        } else {
            if(next != NULL) {
                //Открыть найденый файл
                atomic_inc(&next->reference_count);
                vfs_open(next, flags);
            }
            
            if(curr_node != mount_info->root_node)
                kfree(curr_node);

            kfree(temp);
            return next;
        }
        
        fs_path += len + 1;
    }
}