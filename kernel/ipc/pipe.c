#include "pipe.h"
#include "mem/kheap.h"
#include "string.h"
#include "mem/pmm.h"
#include "kstdlib.h"

struct file_operations pipe_fops_read = {
    .read = pipe_file_read
};

struct file_operations pipe_fops_write = {
    .write = pipe_file_write
};

struct pipe* new_pipe(size_t size)
{
    struct pipe* pipe = (struct pipe*) kmalloc(sizeof(struct pipe)); 
    memset(pipe, 0, sizeof(struct pipe));

    pipe->size = PIPE_SIZE;
    pipe->buffer = P2V(pmm_alloc_page());
    
    return pipe;
}

int pipe_create_files(struct pipe* pipe, int flags, struct file* read_file, struct file* write_file)
{
    read_file->private_data = pipe;
    read_file->mode = INODE_FLAG_PIPE;
    read_file->flags = flags;
    read_file->pos = 0;
    read_file->dentry = NULL;
    read_file->ops = &pipe_fops_read;

    write_file->private_data = pipe;
    write_file->mode = INODE_FLAG_PIPE;
    write_file->flags = flags;
    write_file->pos = 0;
    write_file->dentry = NULL;
    write_file->ops = &pipe_fops_write;

    return 0;
}

void free_pipe(struct pipe* pipe)
{

}

ssize_t pipe_file_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct pipe* pipe_obj = (struct pipe*) file->private_data;
    acquire_spinlock(&pipe_obj->lock);


    release_spinlock(&pipe_obj->lock);
    return 0;
}

ssize_t pipe_file_write(struct file* file, const char* buffer, size_t count, loff_t offset)
{
    struct pipe* pipe_obj = (struct pipe*) file->private_data;
    acquire_spinlock(&pipe_obj->lock);


    release_spinlock(&pipe_obj->lock);
    return 0;
}