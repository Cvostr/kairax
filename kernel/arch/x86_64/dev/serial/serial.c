#include "serial.h"
#include "io.h"
#include "drivers/tty/tty.h"
#include "mem/kheap.h"
#include "fs/devfs/devfs.h"
#include "kairax/stdio.h"
#include "dev/interrupts.h"

#define COM1    0x3F8
#define COM2    0x2F8
#define COM3    0x3E8
#define COM4    0x2E8
#define COM5    0x5F8
#define COM6    0x4F8
#define COM7    0x5E8
#define COM8    0x4E8

#define SPEED_BASE 115200

// Размер символа
#define CSIZE_5 0b00
#define CSIZE_6 0b01
#define CSIZE_7 0b10
#define CSIZE_8 0b11

// Parity
#define PARITY_NO       0
#define PARITY_ODD      0b001000
#define PARITY_EVEN     0b011000
#define PARITY_MARK     0b101000
#define PARITY_SPACE    0b111000

// Modem Control Register
#define MCR_DTR     0b00001
#define MCR_RTS     0b00010
#define MCR_OUT1    0b00100
#define MCR_OUT2    0b01000
#define MCR_LOOP    0b10000

// Line status register
#define LSR_DATA_READY      1
#define LSR_OVERRUN_ERR     0b00010
#define LSR_PARITY_ERR      0b00100
#define LSR_FRAMING_ERR     0b01000
#define LSR_BREAK_INDICATOR 0b10000
#define LSR_TRANS_REG_EMPTY 0b100000

struct serial_state
{
    uint8_t id;
    uint16_t port_offset;
    
    //
    struct pty* pty_ptr;
    struct file *master;
    struct file *slave;
    tcflag_t c_cflag;
};

void serial_init_port(int id, uint16_t offset);
int serial_file_ioctl(struct file* file, uint64_t request, uint64_t arg);
ssize_t serial_file_read(struct file* file, char* buffer, size_t count, loff_t offset);
ssize_t serial_file_write(struct file* file, const char* buffer, size_t count, loff_t offset);

struct file_operations serial_fops = 
{
    .read = serial_file_read,
    .write = serial_file_write,
    .ioctl = serial_file_ioctl
};

#define MAX_SERIAL_PORTS    8
struct serial_state* serial_ports[MAX_SERIAL_PORTS];

void serial_write(uint16_t port_offset, char a)
{
    // Ожидание взведения Transmitter holding register empty
    while ((inb(port_offset + 5) & LSR_TRANS_REG_EMPTY) == 0);

    // Отправка
    outb(port_offset,  a);
}

ssize_t serial_write_from_tty(void* tty, const char *buffer, size_t size)
{
    struct serial_state *state = tty;

    for (size_t i = 0; i < size; i ++)
    {
        serial_write(state->port_offset, buffer[i]);
    }

    return size;
}

void serial_rx_handle(uint16_t port_offset)
{
    uint8_t val = inb(port_offset);

    //printk("Serial IRQ on %x - %c (%i)\n", port_offset, val, val);

    struct serial_state *state = NULL;
    struct serial_state *cur = NULL;

    for (int i = 0; i < MAX_SERIAL_PORTS; i ++)
    {
        cur = serial_ports[i];
        if (cur != NULL && cur->port_offset == port_offset)
        {
            state = cur;
            break;
        }
    }

    if (state == NULL)
    {
        return;
    }

    file_write(state->master, 1, &val);
}

void serial_irq_handler(void* frame, void* data)
{
    int first_port_offset = (int) data;
    int second_port_offset = first_port_offset - 0x10;

    uint8_t lsr_first = inb(first_port_offset + 5);
    uint8_t lsr_second = inb(second_port_offset + 5);

    if (lsr_first != 0xFF && (lsr_first & LSR_DATA_READY) == LSR_DATA_READY)
    {
        serial_rx_handle(first_port_offset);
    }

    if (lsr_second != 0xFF && (lsr_second & LSR_DATA_READY) == LSR_DATA_READY)
    {
        serial_rx_handle(second_port_offset);
    }
}

void serial_init()
{
    serial_init_port(1, COM1);
    serial_init_port(2, COM2);
    serial_init_port(3, COM3);
    serial_init_port(4, COM4);

    register_irq_handler(4, serial_irq_handler, COM1);
    register_irq_handler(3, serial_irq_handler, COM2);
}

#define SERIAL_PROBE_VALUE  0xAD
int serial_cfg_port(int id, uint16_t offset, int speed, int csize, int parity, int test)
{
    // Выключение прерываний
    outb(offset + 1, 0x00);
    // Divisor mode
	outb(offset + 3, 0x80);

    // Скорость
    uint16_t divisor = SPEED_BASE / speed;
    outb(offset + 0, divisor & 0xFF);
    outb(offset + 1, divisor >> 8);

    // Line control register
    uint8_t linectl = csize | parity;
    outb(offset + 3, linectl);

    // FIFO + clear
    outb(offset + 2, 0xC7);

    // Modem Control Register
    outb(offset + 4, (MCR_DTR | MCR_RTS | MCR_OUT2));

    if (test == TRUE)
    {
        // Включаем loopback режим
        outb(offset + 4, (MCR_LOOP | MCR_RTS | MCR_OUT1 | MCR_OUT2));
        // Отправляем тестовый байт
        outb(offset + 0, SERIAL_PROBE_VALUE);

        // На реальных машинах может потребоваться ожидание, пока данные дойдут
        // Если будут проблемы - увеличить время ожидания
        hpet_sleep(1);

        // Принимаем байт и сравниваем с тем, что послали
        uint8_t recvd = inb(offset + 0);
        if (recvd != SERIAL_PROBE_VALUE)
        {
            return FALSE;
        }

        // Возвращаем режим
        outb(offset + 4, (MCR_DTR | MCR_RTS | MCR_OUT2));
    }
    
    // Включить прерывания
    outb(offset + 1, 0x01);

    return TRUE;
}

struct baud_table {
    speed_t tty_val;
	int speed;
};
struct baud_table baud_rates[] = {
	{      B0, 0      },
	{     B50, 50     },
	{     B75, 75     },
	{    B110, 110    },
	{    B134, 134    },
	{    B150, 150    },
	{    B200, 200    },
	{    B300, 300    },
	{    B600, 600    },
	{   B1200, 1200   },
	{   B1800, 1800   },
	{   B2400, 2400   },
	{   B4800, 4800   },
	{   B9600, 9600   },
	{  B19200, 19200  },
	{  B38400, 38400  },
	{  B57600, 57600  },
	{ B115200, 115200 },
	{ B230400, 230400 },
	{ B460800, 460800 },
	{ B921600, 921600 },
};

void parse_cflags(tcflag_t cflags, int* speed, int* csize, int* parity)
{
    speed_t cspeed = cflags & CBAUD;
    for (size_t i = 0; i < sizeof(baud_rates) / sizeof(struct baud_table); ++i) 
    {
		if (cspeed == baud_rates[i].tty_val) 
        {
			*speed = baud_rates[i].speed;
			break;
		}
	}

    *parity = PARITY_NO;
    if ((cflags & PARENB) == PARENB)
    {
        if ((cflags & PARODD) == PARODD)
        {
            *parity = PARITY_ODD;
        }

        *parity = PARITY_EVEN;
    }

    switch (cflags & CSIZE) {
		case CS5: *csize = CSIZE_5; break;
		case CS6: *csize = CSIZE_6; break;
		case CS7: *csize = CSIZE_7; break;
		case CS8: *csize = CSIZE_8; break;
	}
}

void serial_init_port(int id, uint16_t offset)
{
    int available = serial_cfg_port(id, offset, 38400, CSIZE_8, PARITY_NO, TRUE);
    if (available == FALSE)
    {
        // Возможно, порт отсутствует
        return;
    }

    printk("COM%i: Port is available on 0x%x\n", id, offset);

    struct serial_state *state = kmalloc(sizeof(struct serial_state));
    state->id = id;
    state->port_offset = offset;
    // Создание TTY
    tty_create_with_external_master(&state->pty_ptr, &state->master, &state->slave, state, serial_write_from_tty);
    file_acquire(state->slave);
    file_acquire(state->master);

    // Сохранить указатель
    serial_ports[id] = state;

    // Сформировать имя
    char devname[20];
    sprintf(devname, sizeof(devname), "serial%i", id);

    // Добавить символьное устройство
    devfs_add_char_device(devname, &serial_fops, state);
}

ssize_t serial_file_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct serial_state *state = file->inode->private_data;
    return file_read(state->slave, count, buffer);
}

ssize_t serial_file_write(struct file* file, const char* buffer, size_t count, loff_t offset)
{
    struct serial_state *state = file->inode->private_data;
    return file_write(state->slave, count, buffer);
}

int serial_file_ioctl(struct file* file, uint64_t request, uint64_t arg)
{
    struct serial_state *state = file->inode->private_data;
    return file_ioctl(state->slave, request, arg);
}