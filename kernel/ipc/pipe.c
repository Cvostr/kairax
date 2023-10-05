#include "pipe.h"
#include "mem/kheap.h"
#include "string.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "proc/thread_scheduler.h"

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

    write_file->private_data = pipe;
    write_file->mode = INODE_FLAG_PIPE;
    write_file->flags = flags | FILE_OPEN_MODE_WRITE_ONLY;
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
    struct pipe* p = (struct pipe*) file->private_data;
    acquire_spinlock(&p->lock);

    while (p->read_pos == p->write_pos) {
        // Нечего читать - засыпаем
        scheduler_sleep(&p->nreadfds, &p->lock);
    }

    for (size_t i = 0; i < count; i ++) {

        if(p->read_pos == p->write_pos)
            break;
            
        buffer[i] = p->buffer[p->read_pos++ % PIPE_SIZE];
    }

    // Пробуждаем записывающих
    scheduler_wakeup(&p->nwritefds);

    release_spinlock(&p->lock);
    return 0;
}

ssize_t pipe_file_write(struct file* file, const char* buffer, size_t count, loff_t offset)
{
    struct pipe* p = (struct pipe*) file->private_data;
    acquire_spinlock(&p->lock);
    
    for (size_t i = 0; i < count; i ++) {
        
        while (p->write_pos == p->read_pos + PIPE_SIZE) {
            // Больше места нет - пробуждаем читающих
            scheduler_wakeup(&p->nreadfds);
            // И засыпаем сами
            scheduler_sleep(&p->nwritefds, &p->lock);
        }

        // записываем байт
        p->buffer[p->write_pos++ % PIPE_SIZE] = buffer[i];
    }

    // Пробуждаем читающих
    scheduler_wakeup(&p->nreadfds);

    release_spinlock(&p->lock);
    return 0;
}