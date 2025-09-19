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

//#define PTY_CLOSE_LOG

struct pty {
    int pty_id;
    atomic_t refs;

    // termios struct state
    tcflag_t iflag;
    tcflag_t oflag;
    tcflag_t cflag;
    tcflag_t lflag;
    cc_t control_characters[CCSNUM];
    // Размер окна
    struct winsize winsz;
    // Каналы
    struct pipe* master_to_slave;
    struct pipe* slave_to_master;

    // Для случаев, когда master - не файл, а устройство
    void* owner;
    pty_output_write_func_t output_routine;

    // canonical буфер
    char buffer[PTY_LINE_MAX_BUFFER_SIZE];
    int buffer_pos;

    pid_t foreground_pg;
};

tcflag_t tty_get_cflag(struct pty* p_pty)
{
    return p_pty->cflag;
}

void tty_init()
{
    tty_master_fops.close = master_file_close;
    tty_master_fops.write = master_file_write;
    tty_master_fops.read = master_file_read;
    tty_master_fops.ioctl = tty_ioctl;
	
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
    control_characters[VEOL] = 0;   // по умолчанию не указываем
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
#ifdef PTY_CLOSE_LOG
        printk("pty: free()\n");
#endif
        kfree(p_pty);
    }
}

int master_file_close(struct inode *inode, struct file *file)
{
#ifdef PTY_CLOSE_LOG
    printk("tty: master close()\n");
#endif
    struct pty *p_pty = (struct pty *) file->private_data;
    free_pty(p_pty);
    return 0;
}

int slave_file_close(struct inode *inode, struct file *file)
{
#ifdef PTY_CLOSE_LOG
    printk("tty: slave close()\n");
#endif
    struct pty *p_pty = (struct pty *) file->private_data;
    free_pty(p_pty);
    return 0;
}

struct pty* new_tty()
{
    struct pty* p_pty = kmalloc(sizeof(struct pty));
    if (p_pty == NULL)
    {
        return NULL;
    }
    memset(p_pty, 0, sizeof(struct pty));

    // Установить флаги по умолчанию
    p_pty->lflag = (ISIG | ICANON | ECHO | ECHOE | ECHOK | IEXTEN);
    p_pty->oflag = (OPOST | ONLCR);
    p_pty->cflag = (CS8 | B38400);
    tty_fill_ccs(p_pty->control_characters);
    // Размер окна по умолчанию
    p_pty->winsz.ws_col = 80;
    p_pty->winsz.ws_row = 20;

    return p_pty;
}

int tty_create(struct file **master, struct file **slave)
{
    struct pty* p_pty = new_tty();
    if (p_pty == NULL)
    {
        return -ENOMEM;
    }

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

    struct file *fslave = new_file();
    fslave->flags = FILE_OPEN_MODE_READ_WRITE;
    fslave->ops = &tty_slave_fops;
    fslave->private_data = p_pty;
    atomic_inc(&p_pty->refs);

    *master = fmaster;
    *slave = fslave;

    return 0;
}

int tty_create_with_external_master(struct pty** pty, struct file **slave, void* owner, pty_output_write_func_t func)
{
    struct pty* p_pty = new_tty();
    if (p_pty == NULL)
    {
        return -ENOMEM;
    }

    p_pty->owner = owner;
    p_pty->output_routine = func;

    // Создать канал для ведомого
    p_pty->slave_to_master = new_pipe();
    atomic_inc(&p_pty->slave_to_master->ref_count);

    struct file *fslave = new_file();
    fslave->flags = FILE_OPEN_MODE_READ_WRITE;
    fslave->ops = &tty_slave_fops;
    fslave->private_data = p_pty;
    atomic_inc(&p_pty->refs);
    *slave = fslave;

    *pty = p_pty;
    atomic_inc(&p_pty->refs);

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
        case TIOCGPGRP:
            VALIDATE_USER_POINTER_PROTECTION(process, arg, sizeof(pid_t), PAGE_PROTECTION_WRITE_ENABLE)
            *((pid_t*) arg) = p_pty->foreground_pg;
            break;
        case TIOCSTI:
            VALIDATE_USER_POINTER(process, arg, 1);
            tty_output(p_pty, *((unsigned char*) arg));
            break;
        case TIOCGWINSZ:
            // Получить размер окна
            VALIDATE_USER_POINTER_PROTECTION(process, arg, sizeof(struct winsize), PAGE_PROTECTION_WRITE_ENABLE)
            memcpy(arg, &p_pty->winsz, sizeof(struct winsize));
            break;
        case TIOCSWINSZ:
            // Обновить размер окна
            VALIDATE_USER_POINTER(process, arg, sizeof(struct winsize))
            memcpy(&p_pty->winsz, arg, sizeof(struct winsize));
            sys_send_signal_pg(p_pty->foreground_pg, SIGWINCH);
            break;
        case TIOCNOTTY:
            sys_send_signal_pg(p_pty->foreground_pg, SIGHUP);
            sys_send_signal_pg(p_pty->foreground_pg, SIGCONT);
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

            memcpy(p_pty->control_characters, tmios->c_cc, sizeof(cc_t) * CCSNUM);

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

ssize_t tty_output_write(struct pty* p_pty, unsigned char* buffer, size_t size)
{
    pty_output_write_func_t output_routine = p_pty->output_routine;

    if (output_routine != NULL)
    {
        return output_routine(p_pty->owner, buffer, size);
    }

    return pipe_write(p_pty->slave_to_master, buffer, size);
}

void tty_output(struct pty* p_pty, unsigned char chr)
{
    if (p_pty->oflag & OPOST)
    {
        if ((p_pty->oflag & ONLCR) && chr == '\n')
        {
            // Сразу пишем CRLF
            tty_output_write(p_pty, crlf, sizeof(crlf));
            return;
        }

        if ((p_pty->oflag & OCRNL) && chr == '\r')
        {
            chr = '\n';
            tty_output_write(p_pty, &chr, sizeof(chr));
            return;
        }

        if ((p_pty->oflag & OLCUC) && (chr >= 'a' && chr <= 'z'))
        {
            chr = chr + 'A' - 'a';
            tty_output_write(p_pty, &chr, sizeof(chr));
            return;
        }
    }

    tty_output_write(p_pty, &chr, 1);
}

// Запись со стороны приложения
void tty_line_discipline_sw(struct pty* p_pty, const char* buffer, size_t count)
{
    size_t i = 0;
    while (i < count) 
    {
        char chr = buffer[i++];
        tty_output(p_pty, chr);
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

void pty_flush(struct pty* p_pty)
{
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

        // Понижение регистра
        if (
            (p_pty->lflag & IEXTEN) &&
            (p_pty->iflag & IUCLC) &&
            (first_char >= 'A' && first_char <= 'Z')) 
        {
		    first_char = first_char - 'A' + 'a';
	    }

        // Обработка сигнальных управляющих символов
        if ((p_pty->lflag & ISIG) == ISIG)
        {
            if (first_char == p_pty->control_characters[VINTR])
            {
                sys_send_signal_pg(p_pty->foreground_pg, SIGINT);
            }
            else if (first_char == p_pty->control_characters[VQUIT])
            {
                sys_send_signal_pg(p_pty->foreground_pg, SIGQUIT);
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

        char EOL = p_pty->control_characters[VEOL];

        if (first_char == '\r')
        {
            // пока что CR просто выводим, не добавляя в буфер
            tty_output_write(p_pty, &first_char, 1);
        }
        else if ((EOL != 0 && first_char == EOL) || (first_char == '\n'))
        {
            // Нажата кнопка enter
            if ((p_pty->lflag & ECHO) == ECHO)
            {
                // Новая линия CR+LF
                tty_output_write(p_pty, crlf, 2);
            }

            // Отправить в терминал
            pty_flush(p_pty);
        } 
        else if (first_char <= 31 || first_char == 127)
        {
            // Вывод управляющего символа в формате ^C
            char chr = '^';
            tty_output_write(p_pty, &chr, sizeof(chr));
            chr = 'A' + first_char - 1;
            tty_output_write(p_pty, &chr, sizeof(chr));

            // Сброс позиции буфера
            p_pty->buffer_pos = 0;
            memset(p_pty->buffer, 0, PTY_LINE_MAX_BUFFER_SIZE);

            // Эхо в терминал CR + LF
            tty_output_write(p_pty, crlf, 2);
            pty_flush(p_pty);
        }
        else
        {
            if ((p_pty->lflag & ICANON) == ICANON) 
            {
                // Добавить символ в буфер
                pty_linebuffer_append(p_pty, first_char);
            }
            else
            {
                pipe_write(p_pty->master_to_slave, &first_char, sizeof(char));
            }
                
            // Эхо на консоль
            if ((p_pty->lflag & ECHO) == ECHO) 
            {
                tty_output(p_pty, first_char);
            }
        }
    }
}