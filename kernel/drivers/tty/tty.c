#include "tty.h"
#include "mem/kheap.h"
#include "ipc/pipe.h"
#include "sync/spinlock.h"
#include "string.h"
#include "proc/syscalls.h"

struct file_operations tty_master_fops;
struct file_operations tty_slave_fops;

#define PTY_LINE_MAX_BUFFER_SIZE 2048

struct pty {
    int pty_id;
    int lflags;
    struct pipe* master_to_slave;
    struct pipe* slave_to_master;

    char buffer[PTY_LINE_MAX_BUFFER_SIZE];
    int buffer_pos;

    int foreground_pg;
};

#define MAX_PTY_COUNT 64
spinlock_t  ptys_lock = 0;
struct pty* ptys[MAX_PTY_COUNT];

void tty_init()
{
    tty_master_fops.close = master_file_close;
    tty_master_fops.write = master_file_write;
    tty_master_fops.read = master_file_read;
	
    tty_slave_fops.close = slave_file_close;
    tty_slave_fops.read = slave_file_read;
    tty_slave_fops.write = slave_file_write;
    tty_slave_fops.ioctl = tty_ioctl;
}

int master_file_close(struct inode *inode, struct file *file)
{
    printk("tty: master close() not implemented!\n");
    return -1;
}

int slave_file_close(struct inode *inode, struct file *file)
{
    printk("tty: slave close() not implemented!\n");
    return -1;
}

int tty_create(struct file **master, struct file **slave)
{
    struct pty* p_pty = kmalloc(sizeof(struct pty));
    memset(p_pty, 0, sizeof(struct pty));

    p_pty->master_to_slave = new_pipe();
    atomic_inc(&p_pty->master_to_slave->ref_count);
    
    p_pty->slave_to_master = new_pipe();
    atomic_inc(&p_pty->slave_to_master->ref_count);

    struct file *fmaster = new_file();
    fmaster->flags = FILE_OPEN_MODE_READ_WRITE;
    fmaster->ops = &tty_master_fops;
    fmaster->private_data = p_pty;
    *master = fmaster;

    struct file *fslave = new_file();
    fslave->flags = FILE_OPEN_MODE_READ_WRITE;
    fslave->ops = &tty_slave_fops;
    fslave->private_data = p_pty;
    *slave = fslave;

    return 0;
}

ssize_t master_file_write (struct file* file, const char* buffer, size_t count, loff_t offset)
{
    // Запись со стороны эмулятора терминала
    struct pty *p_pty = (struct pty *) file->private_data;
    tty_line_discipline_mw(p_pty, buffer, count);
    return count;
}

ssize_t master_file_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct pty *p_pty = (struct pty *) file->private_data;
    // Неблокирующее чтение
    return pipe_read(p_pty->slave_to_master, buffer, count, 1);
}

ssize_t slave_file_write(struct file* file, const char* buffer, size_t count, loff_t offset)
{
    struct pty *p_pty = (struct pty *) file->private_data;
    tty_line_discipline_sw(p_pty, buffer, count);
    return count;
}

ssize_t slave_file_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct pty *p_pty = (struct pty *) file->private_data;
    return pipe_read(p_pty->master_to_slave, buffer, count, 0);
}

int tty_ioctl(struct file* file, uint64_t request, uint64_t arg)
{
    struct pty *p_pty = (struct pty *) file->private_data;

    switch (request) {
        case TIOCSPGRP:
            p_pty->foreground_pg = arg;
            break;
    }

    return 0;
}

char crlf[2] = {'\r', '\n'};
char remove[3] = {'\b', ' ', '\b'};
char ETX[3] = {'^', 'C'};

void tty_line_discipline_sw(struct pty* p_pty, const char* buffer, size_t count)
{
    size_t i = 0;
    while (i < count) {
        char chr = buffer[i];

        switch (chr) {
            case '\n':
                pipe_write(p_pty->slave_to_master, crlf, 2);
                break;
            default:
                pipe_write(p_pty->slave_to_master, &chr, 1);
        }
        i++;
    }
}

void pty_linebuffer_append(struct pty* p_pty, char c)
{
    if (p_pty->buffer_pos < (PTY_LINE_MAX_BUFFER_SIZE - 1)) {
        p_pty->buffer[p_pty->buffer_pos++] = c;
    }
}

// Дисциплина линии при записи в master со стороны терминала
void tty_line_discipline_mw(struct pty* p_pty, const char* buffer, size_t count)
{
    size_t i = 0;
    while (i < count) {
        char first_char = buffer[i];

        switch (first_char) {
            case '\n':
                // Нажата кнопка enter
                // Эхо в терминал CR + LF
                pipe_write(p_pty->slave_to_master, crlf, 2);
                // Записать буфер в slave
                p_pty->buffer[p_pty->buffer_pos++] = '\n';
                pipe_write(p_pty->master_to_slave, p_pty->buffer, p_pty->buffer_pos);
                // Сброс позиции буфера
                p_pty->buffer_pos = 0;
                break;
            case '\b':
                // Нажата кнопка backspace
                if (p_pty->buffer_pos > 0) {
                    p_pty->buffer_pos--;
                    // BKSP + SPACE + BKSP
                    pipe_write(p_pty->slave_to_master, remove, 3);
                }
                break;
            case 0x3: // ETX
                pipe_write(p_pty->slave_to_master, ETX, 2); // ^C
                // todo : сигнал завершения процесса
                sys_send_signal(p_pty->foreground_pg, SIGINT);
                break;
            default:
                // Добавить символ в буфер
                pty_linebuffer_append(p_pty, first_char);
                // Эхо на консоль
                pipe_write(p_pty->slave_to_master, &first_char, 1);
                break;
        }

        i++;
    }
}