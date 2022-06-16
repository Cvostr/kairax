#include "pic.h"

void pic_outb(uint8_t port, uint8_t data)
{
    outb(port, data);
    io_wait();
}

void init_pic(void)
{
    pic_outb(PORT_PIC_MASTER_CMD, 0x11); // starts the initialization
    pic_outb(PORT_PIC_SLAVE_CMD, 0x11);

    pic_outb(PORT_PIC_MASTER_DATA, INT_IRQ0);      // INT master PIC
    pic_outb(PORT_PIC_SLAVE_DATA, INT_IRQ0 + 0x8); // INT slave PIC

    pic_outb(PORT_PIC_MASTER_DATA, 0x4);
    pic_outb(PORT_PIC_SLAVE_DATA, 0x2);

    pic_outb(PORT_PIC_MASTER_DATA, ICW4_8086);
    pic_outb(PORT_PIC_SLAVE_DATA, ICW4_8086);

    pic_outb(PORT_PIC_MASTER_DATA, 0xFF); // Mask all interrupts
    pic_outb(PORT_PIC_SLAVE_DATA, 0xFF);
}


void pic_unmask(uint8_t irq)
{
    //получение текущей маски
    uint8_t curmask_master = inb(PORT_PIC_MASTER_DATA);
    /* if bit == 0 irq is enable*/
    outb(PORT_PIC_MASTER_DATA, curmask_master & ~(1 << irq));
}

void pic_mask(uint8_t irq)
{
    //получение текущей маски
    uint8_t curmask_master = inb(PORT_PIC_SLAVE_DATA);
    /* if bit == 0 irq is enable*/
    outb(PORT_PIC_MASTER_DATA, curmask_master | (1 << irq));
}
