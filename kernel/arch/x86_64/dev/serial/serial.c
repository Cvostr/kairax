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

struct serial_state
{
    uint8_t id;
    uint16_t port_offset;
    
    //
    void* pty_ptr;
    tcflag_t c_cflag;
};

ssize_t serial_file_read(struct file* file, char* buffer, size_t count, loff_t offset);
ssize_t serial_file_write(struct file* file, const char* buffer, size_t count, loff_t offset);

struct file_operations serial_fops = 
{
    .read = serial_file_read,
    .write = serial_file_write
};

struct serial_state* serial_ports[8];

void serial_rx_handle(uint16_t port_offset)
{
    uint8_t val = inb(port_offset);

    printk("Serial IRQ on %x - %c\n", port_offset, val);
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

void serial_cfg_port(int id, uint16_t offset, int speed, int csize, int parity)
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

    // Включить прерывания
    outb(offset + 1, 0x01);
}

void serial_init_port(int id, uint16_t offset)
{
    serial_cfg_port(id, offset, 38400, CSIZE_8, PARITY_NO);

    struct serial_state *state = kmalloc(sizeof(struct serial_state));
    state->id = id;
    state->port_offset = offset;

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
    return 0;
}

ssize_t serial_file_write(struct file* file, const char* buffer, size_t count, loff_t offset)
{
    return 0;
}