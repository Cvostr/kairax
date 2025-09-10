#include "tty.h"
#include "mem/kheap.h"
#include "ipc/pipe.h"
#include "sync/spinlock.h"
#include "string.h"
#include "proc/syscalls.h"
#include "stdio.h"
#include "cpu/cpu_local.h"
#include "kairax/ctype.h"

struct file_operations tty_master_fops;
struct file_operations tty_slave_fops;

#define PTY_LINE_MAX_BUFFER_SIZE 2048

struct pty {
    int pty_id;
    atomic_t refs;

    // termios struct state
    tcflag_t iflag;
    tcflag_t oflag;
    tcflag_t cflag;
    tcflag_t lflag;
    cc_t control_characters[CCSNUM];

    struct pipe* master_to_slave;
    struct pipe* slave_to_master;

    char buffer[PTY_LINE_MAX_BUFFER_SIZE];
    int buffer_pos;

    int foreground_pg;
};

/*
// Зачем это предполагалось?
#define MAX_PTY_COUNT 64
spinlock_t  ptys_lock = 0;
struct pty* ptys[MAX_PTY_COUNT];
*/

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

void tty_fill_ccs(cc_t *control_characters)
{
    control_characters[VINTR] = ETX;
    control_characters[VQUIT] = FS;
    control_characters[VERASE] = BS;
    control_characters[VKILL] = NAK;
    control_characters[VSUSP] = SUB;
    control_characters[VWERASE] = ETB;
}

void free_pty(struct pty* p_pty)
{
    if (atomic_dec_and_test(&p_pty->refs)) 
    {
        if (atomic_dec_and_test(&p_pty->master_to_slave->ref_count)) 
        {
            free_pipe(p_pty->master_to_slave);
        }

        if (atomic_dec_and_test(&p_pty->slave_to_master->ref_count)) 
        {
            free_pipe(p_pty->slave_to_master);
        }

        printk("pty: free()\n");
        kfree(p_pty);
    }
}

int master_file_close(struct inode *inode, struct file *file)
{
    struct pty *p_pty = (struct pty *) file->private_data;
    free_pty(p_pty);
    printk("tty: master close()\n");
    return -1;
}

int slave_file_close(struct inode *inode, struct file *file)
{
    struct pty *p_pty = (struct pty *) file->private_data;
    free_pty(p_pty);
    printk("tty: slave close()\n");
    return -1;
}

int tty_create(struct file **master, struct file **slave)
{
    struct pty* p_pty = kmalloc(sizeof(struct pty));
    if (p_pty == NULL)
    {
        return -ENOMEM;
    }
    memset(p_pty, 0, sizeof(struct pty));

    // Установить флаги по умолчанию
    p_pty->lflag = (ISIG | ICANON | ECHO | ECHOE | ECHOK);
    p_pty->oflag = (OPOST | ONLCR);
    tty_fill_ccs(p_pty->control_characters);

    // Создать каналы для ведущего и ведомого
    p_pty->master_to_slave = new_pipe();
    atomic_inc(&p_pty->master_to_slave->ref_count);
    
    p_pty->slave_to_master = new_pipe();
    atomic_inc(&p_pty->slave_to_master->ref_count);

    // Создать файловые дескрипторы
    struct file *fmaster = new_file();
    fmaster->flags = FILE_OPEN_MODE_READ_WRITE;
    fmaster->ops = &tty_master_fops;
    fmaster->private_data = p_pty;
    atomic_inc(&p_pty->refs);
    *master = fmaster;

    struct file *fslave = new_file();
    fslave->flags = FILE_OPEN_MODE_READ_WRITE;
    fslave->ops = &tty_slave_fops;
    fslave->private_data = p_pty;
    atomic_inc(&p_pty->refs);
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
    return pipe_read(p_pty->slave_to_master, buffer, count, 0);
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
    struct process* process = cpu_get_current_thread()->process;
    struct pty *p_pty = (struct pty *) file->private_data;

    struct termios* tmios;

    switch (request) {
        case TIOCSPGRP:
            p_pty->foreground_pg = arg;
            break;
        case TCGETS:
            tmios = (struct termios*) arg;
            VALIDATE_USER_POINTER(process, arg, sizeof(struct termios))

            tmios->c_iflag = p_pty->iflag;
            tmios->c_oflag = p_pty->oflag;
            tmios->c_cflag = p_pty->cflag;
            tmios->c_lflag = p_pty->lflag;
            memcpy(tmios->c_cc, p_pty->control_characters, sizeof(cc_t) * CCSNUM);

            break;
        case TCSETS: 
        case TCSETSF:
        case TCSETSW:
            int mode = request - TCSETS;

            // TODO: учесть режим
            tmios = (struct termios*) arg;
            VALIDATE_USER_POINTER(process, arg, sizeof(struct termios))

            p_pty->iflag = tmios->c_iflag;
            p_pty->oflag = tmios->c_oflag;
            p_pty->cflag = tmios->c_cflag;
            p_pty->lflag = tmios->c_lflag;

            // todo: implement returning termios
            break;

        default:
            return -EINVAL;
    }

    return 0;
}

char crlf[2] = {'\r', '\n'};
char remove[3] = {'\b', ' ', '\b'};
char ETX_ECHO[2] = {'^', 'C'};
char FS_ECHO[2] = {'^', '\\'};

// Запись со стороны приложения
void tty_line_discipline_sw(struct pty* p_pty, const char* buffer, size_t count)
{
    size_t i = 0;
    while (i < count) 
    {
        char chr = buffer[i++];

        if (p_pty->oflag & OPOST)
        {
            if ((p_pty->oflag & ONLCR) && chr == '\n')
            {
                // Сразу пишем CRLF
                pipe_write(p_pty->slave_to_master, crlf, sizeof(crlf));
                return;
            }

            if ((p_pty->oflag & OCRNL) && chr == '\r')
            {
                chr = '\n';
                pipe_write(p_pty->slave_to_master, &chr, sizeof(chr));
                return;
            }

            if ((p_pty->oflag & OLCUC) && (chr >= 'a' && chr <= 'z'))
            {
                chr = chr + 'A' - 'a';
                pipe_write(p_pty->slave_to_master, &chr, sizeof(chr));
                return;
            }
        }

        pipe_write(p_pty->slave_to_master, &chr, 1);
    }   
}

void pty_linebuffer_append(struct pty* p_pty, char c)
{
    if (p_pty->buffer_pos < (PTY_LINE_MAX_BUFFER_SIZE - 1)) {
        p_pty->buffer[p_pty->buffer_pos++] = c;
    }
}

void pty_remove_last_char(struct pty* p_pty)
{
    if (p_pty->buffer_pos > 0) {
        p_pty->buffer_pos--;
        // BKSP + SPACE + BKSP
        pipe_write(p_pty->slave_to_master, remove, sizeof(remove));
    }
}

void pty_newline_flush(struct pty* p_pty)
{
    // Нажата кнопка enter
    // Эхо в терминал CR + LF
    pipe_write(p_pty->slave_to_master, crlf, 2);
    // Записать буфер в slave
    p_pty->buffer[p_pty->buffer_pos++] = '\n';
    pipe_write(p_pty->master_to_slave, p_pty->buffer, p_pty->buffer_pos);
    // Сброс позиции буфера
    p_pty->buffer_pos = 0;
}

// Дисциплина линии при записи в master со стороны терминала
void tty_line_discipline_mw(struct pty* p_pty, const char* buffer, size_t count)
{
    size_t i = 0;
    while (i < count) 
    {
        char first_char = buffer[i++];

        if ((p_pty->iflag & ISTRIP) == ISTRIP)
        {
            first_char &= 0x7F;
        }

        if (((p_pty->iflag & IGNCR) == IGNCR) && (first_char == '\r'))
        {
            // Ignore Carriage Return
            continue;
        }

        // Преобразования CR -> LF, если необходимо
        char converted = first_char;
        if (((p_pty->iflag & INLCR) == INLCR) && (first_char == '\n'))
        {
            converted = '\r';
        }
        if (((p_pty->iflag & ICRNL) == ICRNL) && (first_char == '\r'))
        {
            converted = '\n';
        }
        first_char = converted;

        // Обработка сигнальных управляющих символов
        if ((p_pty->lflag & ISIG) == ISIG)
        {
            if (first_char == p_pty->control_characters[VINTR])
            {
                sys_send_signal(p_pty->foreground_pg, SIGINT);
            }
            else if (first_char == p_pty->control_characters[VQUIT])
            {
                sys_send_signal(p_pty->foreground_pg, SIGQUIT);
            }
            else if (first_char == p_pty->control_characters[VSUSP])
            {
                
            }
        }

        // При включенном канонiчном режиме
        if ((p_pty->lflag & ICANON) == ICANON)
        {
            // Удаление строки
            if ((first_char == p_pty->control_characters[VKILL]) && ((p_pty->lflag & ECHOK) == ECHOK))
            {
                // Очищаем всю строку
                while (p_pty->buffer_pos > 0) 
                {
                    pty_remove_last_char(p_pty);
                }

                continue;
            }

            // Удаление символа
            if ((first_char == p_pty->control_characters[VERASE]) && (p_pty->lflag & ECHOE) == ECHOE)
            {
                pty_remove_last_char(p_pty);
                continue;
            }

            // Удаление слова
            if ((first_char == p_pty->control_characters[VWERASE]) && (p_pty->lflag & ECHOE) == ECHOE)
            {
                // Сначала очищаем пробелы
                while (isspace(p_pty->buffer[p_pty->buffer_pos]) && (p_pty->buffer_pos > 0)) 
                {
                    pty_remove_last_char(p_pty);
                }

                // Очищаем слово
                while (!isspace(p_pty->buffer[p_pty->buffer_pos]) && (p_pty->buffer_pos > 0)) 
                {
                    pty_remove_last_char(p_pty);
                }

                continue;
            }
        }

        if (first_char == '\r')
        {
            // пока что CR просто выводим, не добавляя в буфер
            pipe_write(p_pty->slave_to_master, &first_char, 1);
            break;
        }
        else if (first_char == '\n')
        {
            // Новая линия CR+LF
            pty_newline_flush(p_pty);
        } 
        else if (first_char <= 31 || first_char == 127)
        {
            // Вывод управляющего символа в формате ^C
            char chr = '^';
            pipe_write(p_pty->slave_to_master, &chr, sizeof(chr));
            chr = 'A' + first_char - 1;
            pipe_write(p_pty->slave_to_master, &chr, sizeof(chr));
            // Новая линия CR+LF
            pty_newline_flush(p_pty);
        }
        else
        {
            // Добавить символ в буфер
            pty_linebuffer_append(p_pty, first_char);
                
            // Эхо на консоль
            if ((p_pty->lflag & ECHO) == ECHO) 
            {
                pipe_write(p_pty->slave_to_master, &first_char, 1);
            }
        }
    }
}