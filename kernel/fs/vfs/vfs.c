#include "vfs.h"
#include "string.h"
#include "filesystems.h"
#include "mem/kheap.h"

#define MAX_MOUNTS 100

vfs_mount_info_t* vfs_mounts[MAX_MOUNTS];

vfs_inode_t* new_vfs_inode(){
    vfs_inode_t* result = kmalloc(sizeof(vfs_inode_t));
    memset(result, 0, sizeof(vfs_inode_t));
    return result;
}

dirent_t* new_vfs_dirent(){
    dirent_t* result = kmalloc(sizeof(dirent_t));
    memset(result, 0, sizeof(vfs_inode_t));
    return result;
}

void vfs_init(){
    memset(vfs_mounts, 0, sizeof(vfs_mount_info_t*) * MAX_MOUNTS);
}

vfs_mount_info_t* new_vfs_mount_info(){
    vfs_mount_info_t* result = (vfs_mount_info_t*)kmalloc(sizeof(vfs_mount_info_t));
    memset(result, 0, sizeof(vfs_mount_info_t));
    return result;
}

int vfs_get_free_mount_info_pos(){
    for(int i = 0; i < MAX_MOUNTS; i ++){
        if(vfs_mounts[i] == NULL)
            return i;
    }

    return -1;
}

int vfs_mount(char* mount_path, drive_partition_t* partition){
    if(vfs_get_mounted_partition(mount_path) != NULL){
        return -1;  //Данный путь уже используется
    }
    int mount_pos = vfs_get_free_mount_info_pos();
    if(mount_pos == -1)
        return -2;  //Не найдено место

    vfs_mount_info_t* mount_info = new_vfs_mount_info();
    mount_info->partition = partition;
    mount_info->filesystem = filesystem_get_by_name("ext2"); //Захардкожено!!!
    strcpy(mount_info->mount_path, mount_path);
    //вызов функции монтирования, если она определена
    if(mount_info->filesystem->mount)
        mount_info->root_node = mount_info->filesystem->mount(partition);

    //Удалить / в конце, если есть
    int path_len = strlen(mount_info->mount_path);
    if(mount_info->mount_path[path_len - 1] == '/')
        mount_info->mount_path[path_len - 1] = '\0';

    vfs_mounts[mount_pos] = mount_info;
}

int vfs_unmount(char* mount_path){
    for(int i = 0; i < MAX_MOUNTS; i ++){
        if(strcmp(vfs_mounts[i]->mount_path, mount_path) == 0){
            kfree(vfs_mounts[i]);
            vfs_mounts[i] = NULL;
            return 1;
        }
    }

    return -1;//Данный путь не использовался
}

vfs_mount_info_t* vfs_get_mounted_partition(char* mount_path){
    for(int i = 0; i < MAX_MOUNTS; i ++){
        if(vfs_mounts[i] == NULL)
            continue;
        if(strcmp(vfs_mounts[i]->mount_path, mount_path) == 0){
            return vfs_mounts[i];
        }
    }
    return NULL;
}

void path_to_slash(char* path){
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

vfs_mount_info_t* vfs_get_mounted_partition_split(char* path, int* offset){
    int path_len = strlen(path);
    char* temp = kmalloc(path_len + 1);
    strcpy(temp, path);

    vfs_mount_info_t* result = NULL;
    while(result == NULL){
        result = vfs_get_mounted_partition(temp);

        if(result == NULL){
            if(strlen(temp) == 0)
                return NULL;
            path_to_slash(temp);
        }
    }

    *offset = strlen(temp);

    kfree(temp);

    return result;
}

vfs_mount_info_t** vfs_get_mounts(){
    return vfs_mounts;
}

uint32_t vfs_read(vfs_inode_t* file, uint32_t offset, uint32_t size, char* buffer){
    if(file)
        if(file->operations.read)
            return file->operations.read(file, offset, size, buffer);
}

vfs_inode_t* vfs_finddir(vfs_inode_t* node, char* name){
    if(node)
        if(node->operations.finddir)
            return node->operations.finddir(node, name);
}

vfs_inode_t* vfs_readdir(vfs_inode_t* node, uint32_t index){
    if(node)
        if(node->operations.readdir)
            return node->operations.readdir(node, index);
}

void vfs_open(vfs_inode_t* node, uint32_t flags){
    if(node)
        if(node->operations.open)
            node->operations.open(node, flags);
}

void vfs_close(vfs_inode_t* node){
    if(node)
        if(node->operations.close)
            node->operations.close(node);
}

vfs_inode_t* vfs_fopen(const char* path, uint32_t flags){
    int offset = 0;
    vfs_mount_info_t* mount_info = vfs_get_mounted_partition_split(path, &offset);
    vfs_inode_t* curr_node = mount_info->root_node;

    offset += 1;
    char* fs_path = path + offset;
    char* temp = (char*)kmalloc(strlen(fs_path) + 1);

    if(strlen(fs_path) == 0){
        return curr_node;
    }

    while(strlen(fs_path) > 0){
        int len = 0;
        int is_dir = 0;
        char* slash_pos = strchr(fs_path, '/');
        if(slash_pos != NULL){ //Это директория
            len = slash_pos - fs_path;
            is_dir = 1;
        }else 
            len = strlen(fs_path);
        strncpy(temp, fs_path, len);

        vfs_inode_t* next = vfs_finddir(curr_node, temp);
        if(is_dir == 1){
            if(next != NULL){
                if(curr_node != mount_info->root_node)
                    kfree(curr_node);
                curr_node = next;
            }else {
                kfree(temp);
                return NULL;
            }
        }else{
            if(next != NULL){
                //Открыть найденый файл
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