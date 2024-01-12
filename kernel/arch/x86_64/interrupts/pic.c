#include "pic.h"

void pic_outb(uint8_t port, uint8_t data)
{
    outb(port, data);
    io_wait();
}

void init_pic(void)
{
    // Начало
    pic_outb(PORT_PIC_MASTER_CMD, 0x11);
    pic_outb(PORT_PIC_SLAVE_CMD, 0x11);

    // Переместить прерывания за 32 чтобы не было конфликтов с исключениями
    pic_outb(PORT_PIC_MASTER_DATA, INT_IRQ0);      
    pic_outb(PORT_PIC_SLAVE_DATA, INT_IRQ0 + 0x8);

    pic_outb(PORT_PIC_MASTER_DATA, 0x4);
    pic_outb(PORT_PIC_SLAVE_DATA, 0x2);

    pic_outb(PORT_PIC_MASTER_DATA, ICW4_8086);
    pic_outb(PORT_PIC_SLAVE_DATA, ICW4_8086);

    pic_mask_all();
}

void pic_mask_all()
{
    pic_outb(PORT_PIC_MASTER_DATA, 0xFF);
    pic_outb(PORT_PIC_SLAVE_DATA, 0xFF);
}