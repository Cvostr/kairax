#include "file.h"
#include "mem/kheap.h"
#include "string.h"

file_t* new_file()
{
    file_t* file = kmalloc(sizeof(file_t));
    memset(file, 0, sizeof(file_t));
    return file;
}

void file_close(file_t* file) 
{
    inode_close(file->inode);
    kfree(file);
}