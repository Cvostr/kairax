#include "pipe.h"
#include "mem/kheap.h"
#include "string.h"
#include "mem/pmm.h"
#include "kstdlib.h"

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
    if (pipe == NULL) {
        return NULL;
    }

    memset(pipe, 0, sizeof(struct pipe));

    pipe->buffer = P2V(pmm_alloc_page());
    if (pipe->buffer == NULL) {
        kfree(pipe);
        return NULL;
    }
    
    return pipe;
}

void free_pipe(struct pipe* pipe)
{
    pipe->dead = 1;

    pmm_free_page(V2P(pipe->buffer));
    kfree(pipe);
}

int pipe_create_files(struct pipe* pipe, int flags, struct file* read_file, struct file* write_file)
{
    struct inode* ino = new_vfs_inode();
    ino->mode = INODE_FLAG_PIPE;

    read_file->private_data = pipe;
    read_file->flags = flags | FILE_OPEN_MODE_READ_ONLY;
    read_file->pos = 0;
    read_file->dentry = NULL;
    read_file->inode = ino;
    read_file->ops = &pipe_fops_read;
    pipe->nreadfds++;

    write_file->private_data = pipe;
    write_file->flags = flags | FILE_OPEN_MODE_WRITE_ONLY;
    write_file->pos = 0;
    write_file->dentry = NULL;
    write_file->inode = ino;
    write_file->ops = &pipe_fops_write;
    pipe->nwritefds++;

    atomic_inc(&pipe->ref_count);
    atomic_inc(&pipe->ref_count);

    inode_open(ino, 0);
    inode_open(ino, 0);

    return 0;
}

ssize_t pipe_read(struct pipe* pipe, char* buffer, size_t count, int nonblock)
{
    size_t i;

    // Канал уничтожен
    if (pipe->dead == 1) 
    {
        // TODO : SIGPIPE
        return -ESPIPE;
    }

    acquire_spinlock(&pipe->lock);

    while (pipe->read_pos == pipe->write_pos) {

        // Закрыт ли дескриптор записи?
        if ((pipe->check_ends == 1 && pipe->nwritefds == 0))
        {
            // TODO : SIGPIPE
            i = -EPIPE;
            goto exit;
        }

        if (nonblock == 1) {
            i = 0;
            goto exit;
        }

        // Нечего читать - засыпаем
        release_spinlock(&pipe->lock);
        scheduler_sleep_intrusive(&pipe->readb.head, &pipe->readb.tail, &pipe->readb.lock);
        acquire_spinlock(&pipe->lock);
    }

    for (i = 0; i < count; i ++) {

        if (pipe->read_pos == pipe->write_pos)
            break;
            
        buffer[i] = pipe->buffer[pipe->read_pos++ % PIPE_SIZE];
    }

    // Пробуждаем записывающих
    scheduler_wakeup_intrusive(&pipe->writeb.head, &pipe->writeb.tail, &pipe->writeb.lock, INT_MAX);

exit:
    release_spinlock(&pipe->lock);
    return i;
}

ssize_t pipe_write(struct pipe* pipe, const char* buffer, size_t count)
{
    if (pipe->dead == 1) 
    {
        // TODO : SIGPIPE
        return -ESPIPE;
    }

    // Закрыты все читающие
    if ((pipe->check_ends == 1 && pipe->nreadfds == 0))
    {
        // TODO : SIGPIPE
        return -EPIPE;
    }

    acquire_spinlock(&pipe->lock);

    for (size_t i = 0; i < count; i ++) {
        
        while (pipe->write_pos == pipe->read_pos + PIPE_SIZE) {
            // Больше места нет - пробуждаем читающих
            scheduler_wakeup_intrusive(&pipe->readb.head, &pipe->readb.tail, &pipe->readb.lock, INT_MAX);
            // И засыпаем сами
            release_spinlock(&pipe->lock);
            scheduler_sleep_intrusive(&pipe->writeb.head, &pipe->writeb.tail, &pipe->lock);
            acquire_spinlock(&pipe->lock);
        }

        // записываем байт
        pipe->buffer[pipe->write_pos++ % PIPE_SIZE] = buffer[i];
    }

    // Пробуждаем читающих  
    scheduler_wakeup_intrusive(&pipe->readb.head, &pipe->readb.tail, &pipe->readb.lock, INT_MAX);

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

    if (atomic_dec_and_test(&p->ref_count)) {
        free_pipe(p);
    }

    return 0;
}