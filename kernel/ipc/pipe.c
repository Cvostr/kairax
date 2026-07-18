#include "pipe.h"
#include "mem/kheap.h"
#include "string.h"
#include "mem/pmm.h"
#include "kstdlib.h"

struct file_operations pipe_fops_read = {
    .read = pipe_file_read,
    .close = pipe_file_close_read,
    .poll = pipe_poll
};

struct file_operations pipe_fops_write = {
    .write = pipe_file_write,
    .close = pipe_file_close_write,
    .poll = pipe_poll
};

/// --------- Для именованных каналов ------------
struct file_operations pipe_bootstrap_fops = {
    .open = pipe_bootstrap_open
};

struct file_operations namedpipe_fops = {
    .read = pipe_file_read,
    .write = pipe_file_write,
    .close = pipe_namedfile_close,
    .poll = pipe_poll
};
/// ---------------------------------------------

struct pipe* new_pipe()
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

void acquire_pipe(struct pipe* pipe)
{
    atomic_inc(&pipe->ref_count);
}

void free_pipe(struct pipe* pipe)
{
    if (atomic_dec_and_test(&pipe->ref_count)) 
    {
        pmm_free_page(V2P(pipe->buffer));
        kfree(pipe);
    }
}

int pipe_create_files(struct pipe* pipe, int flags, struct file* read_file, struct file* write_file)
{
    struct inode* ino = new_vfs_inode();
    ino->mode = INODE_FLAG_PIPE;

    acquire_pipe(pipe);
    read_file->private_data = pipe;
    read_file->flags = flags | FILE_OPEN_MODE_READ_ONLY;
    read_file->pos = 0;
    read_file->dentry = NULL;
    read_file->inode = ino;
    read_file->ops = &pipe_fops_read;
    pipe->nreadfds++;

    acquire_pipe(pipe);
    write_file->private_data = pipe;
    write_file->flags = flags | FILE_OPEN_MODE_WRITE_ONLY;
    write_file->pos = 0;
    write_file->dentry = NULL;
    write_file->inode = ino;
    write_file->ops = &pipe_fops_write;
    pipe->nwritefds++;

    inode_open(ino, 0);
    inode_open(ino, 0);

    return 0;
}

ssize_t pipe_read(struct pipe* pipe, char* buffer, size_t count, int nonblock)
{
    size_t i;

    acquire_spinlock(&pipe->lock);

    while (pipe->read_pos == pipe->write_pos) 
    {
        // Закрыт ли дескриптор записи?
        if ((pipe->check_ends == 1 && pipe->nwritefds == 0))
        {
            // EOF
            i = 0;
            goto exit;
        }

        if (nonblock == 1) {
            i = 0;
            goto exit_wake_writers;
        }

        // Нечего читать - засыпаем
        release_spinlock(&pipe->lock);
        if (scheduler_sleep_on(&pipe->readb) == 1)
        {
            // Нас разбудили сигналом - выходим
            return -EINTR;
        }

        acquire_spinlock(&pipe->lock);
    }

    for (i = 0; i < count; i ++) {

        if (pipe->read_pos == pipe->write_pos)
            break;
            
        buffer[i] = pipe->buffer[pipe->read_pos++ % PIPE_SIZE];
    }

exit_wake_writers:
    // Пробуждаем записывающих
    scheduler_wake(&pipe->writeb, INT_MAX);

    poll_wakeall(&pipe->poll_wq);

exit:
    release_spinlock(&pipe->lock);
    return i;
}

ssize_t pipe_write(struct pipe* pipe, const char* buffer, size_t count)
{
    // Закрыты все читающие
    if (pipe->check_ends == 1 && pipe->nreadfds == 0)
    {
        // Пробуем убить себя же сигналом
        process_send_signal_self(SIGPIPE);
        return -EPIPE;
    }

    acquire_spinlock(&pipe->lock);

    for (size_t i = 0; i < count; i ++) {
        
        while (pipe->write_pos == pipe->read_pos + PIPE_SIZE) {
            // Больше места нет - пробуждаем читающих
            scheduler_wake(&pipe->readb, INT_MAX);
            // И засыпаем сами
            release_spinlock(&pipe->lock);
            if (scheduler_sleep_on(&pipe->writeb) == 1)
            {
                // Нас разбудили сигналом - выходим
                return -EINTR;
            }

            acquire_spinlock(&pipe->lock);
        }

        // записываем байт
        pipe->buffer[pipe->write_pos++ % PIPE_SIZE] = buffer[i];
    }

    // Пробуждаем читающих  
    scheduler_wake(&pipe->readb, INT_MAX);

    poll_wakeall(&pipe->poll_wq);

    release_spinlock(&pipe->lock);
    return count;
}

short pipe_poll(struct file *file, struct poll_ctl *pctl)
{
    short poll_mask = 0;
    struct pipe* p = (struct pipe*) file->private_data;

    poll_wait(pctl, file, &p->poll_wq);

    int writefd = (file->flags & FILE_OPEN_MODE_WRITE_ONLY) == FILE_OPEN_MODE_WRITE_ONLY;

    if ((writefd == FALSE) && (p->read_pos < p->write_pos))
        poll_mask |= (POLLIN | POLLRDNORM);
    if ((writefd == TRUE) && (p->write_pos < p->read_pos + PIPE_SIZE))
        poll_mask |= (POLLOUT | POLLWRNORM);

    // Закрылся файл для записи
    if (writefd == FALSE && p->nwritefds == 0)
        poll_mask |= POLLHUP;
    // Закрылся файл для чтения
    if (writefd == TRUE && p->nreadfds == 0)
        poll_mask |= POLLERR;
    
    return poll_mask;
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

int pipe_file_close_read(struct inode *inode, struct file *file)
{
    struct pipe* p = (struct pipe*) file->private_data;
    
    p->nreadfds --;
    // намеренно никого не будим

    // Будим наблюдающих
    poll_wakeall(&p->poll_wq);

    // уменьшение ссылок на объект pipe
    free_pipe(p);

    return 0;
}

int pipe_file_close_write(struct inode *inode, struct file *file)
{
    struct pipe* p = (struct pipe*) file->private_data;
    
    p->nwritefds --;
    // Будим читающих, чтобы вышли с EOF
    scheduler_wake(&p->readb, INT_MAX);

    // Будим наблюдающих
    poll_wakeall(&p->poll_wq);

    // уменьшение ссылок на объект pipe
    free_pipe(p);

    return 0;
}

/// --------- Для именованных каналов ------------
void inode_setup_namedpipe_fops(struct inode *inode)
{
    inode->file_ops = &pipe_bootstrap_fops;
}

int pipe_bootstrap_open(struct inode *inode, struct file *file)
{
    struct pipe *pipe;
    int read = file_allow_read(file);
    int write = file_allow_write(file);

    acquire_spinlock(&inode->spinlock);

    if (inode->private_data == NULL)
    {
        pipe = new_pipe();
        pipe->check_ends = 1;
        acquire_pipe(pipe);
        inode->private_data = pipe;
    }
    else
    {
        pipe = inode->private_data;
    }

    release_spinlock(&inode->spinlock);

    // добавить pipe к файлу - дескриптору с увеличением счетчика ссылок
    acquire_pipe(pipe);
    file->private_data = pipe;
    // нагло подменяем структуру операций для уже созданного file
    file->ops = &namedpipe_fops;

    acquire_spinlock(&pipe->lock);
    if (read == TRUE)
        pipe->nreadfds ++;
    if (write == TRUE)
        pipe->nwritefds ++;

    if ((read == TRUE && pipe->nwritefds == 0) || (write == TRUE && pipe->nreadfds == 0))
    {
        // компаньон еще не присоединен

        // если явно указан неблокирующий режим
        if ((file->flags & FILE_FLAG_NONBLOCK) == FILE_FLAG_NONBLOCK)
        {
            if (read == TRUE) {
                return 0;
            } else if (write == TRUE) {
                return -ENXIO;
            }
        }

        release_spinlock(&pipe->lock);
        if (scheduler_sleep_on(&pipe->connb) == WAKE_SIGNAL)
        {
            // Нас разбудили сигналом - выходим
            // ничего не закрываем, не сбрасываем, не уменьшаем
            // так как после возврата файл закроется, а операции уже переопределены
            // всё, что нужно, будет сделано в pipe_namedfile_close
            return -EINTR;
        }

        acquire_spinlock(&pipe->lock);
    } 
    else
    {
        // Будим ожидающих подключения
        scheduler_wake(&pipe->connb, INT_MAX);
    }

    release_spinlock(&pipe->lock);

    return 0;
}

int pipe_namedfile_close(struct inode *inode, struct file *file)
{
    struct pipe* p = (struct pipe*) file->private_data;
 
    int read = file_allow_read(file);
    int write = file_allow_write(file);

    if (write == TRUE)
    {
        p->nwritefds --;
        // Будим читающих, чтобы вышли с EOF
        scheduler_wake(&p->readb, INT_MAX);
    }

    if (read == TRUE)
    {
        p->nreadfds --;
        // намеренно никого не будим
    }

    if (p->nwritefds == 0 && p->nreadfds == 0)
    {
        inode->private_data = NULL;
        free_pipe(p);
    }

    // Будим наблюдающих
    poll_wakeall(&p->poll_wq);

    // уменьшение ссылок на объект pipe
    free_pipe(p);

    return 0;
}