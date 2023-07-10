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

void file_set_name_from_path(file_t* file, const char* path)
{
    char* current_slash_pos = strchr(path, '/');
    char* prev_slash_pos = NULL;

    while (current_slash_pos != NULL) {
        prev_slash_pos = current_slash_pos + 1;
        current_slash_pos = strchr(current_slash_pos + 1, '/');

        if (current_slash_pos == NULL) {
            strcpy(file->name, prev_slash_pos);
            break;
        }
    }
}