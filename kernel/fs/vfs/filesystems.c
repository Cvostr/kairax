#include "filesystems.h"
#include "mem/kheap.h"
#include "string.h"
#include "list/list.h"

list_t* filesystems = NULL;

filesystem_t* new_filesystem(){
    filesystem_t* filesystem = (filesystem_t*)kmalloc(sizeof(filesystem_t));
    memset(filesystem, 0, sizeof(filesystem_t));
    return filesystem;
}

void filesystem_register(filesystem_t* filesystem){
    if(filesystems == NULL)
        filesystems = create_list();

    list_add(filesystems, filesystem);
}

void filesystem_unregister(filesystem_t* filesystem){

}

filesystem_t* filesystem_get_by_name(char* name){
    uint32_t fs_size = filesystems->size;
    for(uint32_t i = 0; i < fs_size; i ++){
        filesystem_t* fs = list_get(filesystems, i);
        if(strcmp(fs->name, name) == 0)
            return fs;
    }

    return NULL;
}