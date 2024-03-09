#include "pipe.h"
#include "mem/kheap.h"
#include "string.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "proc/thread_scheduler.h"

struct file_operations pipe_fops_read = {
    .read = pipe_file_read,
    .close = pipe_file_close
};

struct file_operations pipe_fops_write = {
    .write = pipe_file_write,
    .close = pipe_file_close
};

struct pipe* new_pipe(size_t size)
{
    struct pipe* pipe = (struct pipe*) kmalloc(sizeof(struct pipe)); 
    memset(pipe, 0, sizeof(struct pipe));

    pipe->buffer = P2V(pmm_alloc_page());
    
    return pipe;
}

int pipe_create_files(struct pipe* pipe, int flags, struct file* read_file, struct file* write_file)
{
    read_file->private_data = pipe;
    read_file->mode = INODE_FLAG_PIPE;
    read_file->flags = flags | FILE_OPEN_MODE_READ_ONLY;
    read_file->pos = 0;
    read_file->dentry = NULL;
    read_file->ops = &pipe_fops_read;
    pipe->nreadfds++;

    write_file->private_data = pipe;
    write_file->mode = INODE_FLAG_PIPE;
    write_file->flags = flags | FILE_OPEN_MODE_WRITE_ONLY;
    write_file->pos = 0;
    write_file->dentry = NULL;
    write_file->ops = &pipe_fops_write;
    pipe->nwritefds++;

    atomic_inc(&pipe->ref_count);
    atomic_inc(&pipe->ref_count);

    return 0;
}

void free_pipe(struct pipe* pipe)
{
    acquire_spinlock(&pipe->lock);


    release_spinlock(&pipe->lock);
}

ssize_t pipe_read(struct pipe* pipe, char* buffer, size_t count, int nonblock)
{
    size_t i;
    acquire_spinlock(&pipe->lock);

    while (pipe->read_pos == pipe->write_pos) {

        if (nonblock == 1) {
            i = 0;
            goto exit;
        }

        printk("BLK %i %i ", pipe->write_pos, pipe->read_pos);
        // Нечего читать - засыпаем
        scheduler_sleep(&pipe->nreadfds, &pipe->lock);
    }

    for (i = 0; i < count; i ++) {

        if (pipe->read_pos == pipe->write_pos)
            break;
            
        buffer[i] = pipe->buffer[pipe->read_pos++ % PIPE_SIZE];
    }

    // Пробуждаем записывающих
    scheduler_wakeup(&pipe->nwritefds);

exit:
    release_spinlock(&pipe->lock);
    return i;
}

ssize_t pipe_write(struct pipe* pipe, const char* buffer, size_t count)
{
    acquire_spinlock(&pipe->lock);
    for (size_t i = 0; i < count; i ++) {
        
        while (pipe->write_pos == pipe->read_pos + PIPE_SIZE) {
            printk("OBER %i %i", pipe->write_pos, pipe->read_pos);
            // Больше места нет - пробуждаем читающих
            scheduler_wakeup(&pipe->nreadfds);
            // И засыпаем сами
            scheduler_sleep(&pipe->nwritefds, &pipe->lock);
        }

        // записываем байт
        pipe->buffer[pipe->write_pos++ % PIPE_SIZE] = buffer[i];
    }

    // Пробуждаем читающих
    scheduler_wakeup(&pipe->nreadfds);

    release_spinlock(&pipe->lock);
    return count;
}

ssize_t pipe_file_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct pipe* p = (struct pipe*) file->private_data;
    return pipe_read(p, buffer, count, file->flags & FILE_FLAG_NONBLOCK);
}

ssize_t pipe_file_write(struct file* file, const char* buffer, size_t count, loff_t offset)
{
    struct pipe* p = (struct pipe*) file->private_data;
    return pipe_write(p, buffer, count);
}

int pipe_file_close(struct inode *inode, struct file *file)
{
    struct pipe* p = (struct pipe*) file->private_data;
    
    if (file->flags & FILE_OPEN_MODE_READ_ONLY) {

        p->nreadfds --;

    } else if (file->flags & FILE_OPEN_MODE_WRITE_ONLY) {
        p->nwritefds --;
    }

    atomic_dec(&p->ref_count);

    return 0;
}