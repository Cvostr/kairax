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

int serial_file_ioctl(struct file* file, uint64_t request, uint64_t arg);
ssize_t serial_file_read(struct file* file, char* buffer, size_t count, loff_t offset);
ssize_t serial_file_write(struct file* file, const char* buffer, size_t count, loff_t offset);

struct file_operations serial_fops = 
{
    .read = serial_file_read,
    .write = serial_file_write,
    .ioctl = serial_file_ioctl
};

struct serial_state* serial_ports[8];

void serial_write(uint16_t port_offset, char a)
{
    // Ожидание взведения Transmitter holding register empty
    while ((inb(port_offset + 5) & LSR_TRANS_REG_EMPTY) == 0);

    // Отправка
    outb(port_offset,  a);
}

void serial_rx_handle(uint16_t port_offset)
{
    uint8_t val = inb(port_offset);

    printk("Serial IRQ on %x - %c\n", port_offset, val);

    serial_write(port_offset, val);
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

void serial_init_port(int id, uint16_t offset)
{
    int available = serial_cfg_port(id, offset, 38400, CSIZE_8, PARITY_NO, 1);
    if (available == FALSE)
    {
        // Возможно, порт отсутствует
        printk("COM%i: Port is unavailable\n", id);
        return;
    }

    struct serial_state *state = kmalloc(sizeof(struct serial_state));
    state->id = id;
    state->port_offset = offset;
    // Создание TTY
    tty_create(&state->pty_ptr, &state->master, &state->slave);
    file_acquire(state->master);
    file_acquire(state->slave);

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