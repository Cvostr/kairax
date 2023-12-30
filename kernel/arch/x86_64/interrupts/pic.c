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

    // Дополнительный PIC прицеплен ко второму входу главного.
    // Нужно снять маску, чтобы получать прерывания >= IRQ8
    pic_unmask(2);
}

void pic_unmask(uint8_t irq)
{
    uint8_t port = PORT_PIC_MASTER_DATA;

    if (irq >= 8) {
        port = PORT_PIC_SLAVE_DATA;
        irq -= 8;
    }

    uint8_t curmask = inb(port);
    outb(port, curmask & ~(1 << irq));
}

void pic_mask(uint8_t irq)
{
    uint8_t port = PORT_PIC_MASTER_DATA;

    if (irq >= 8) {
        port = PORT_PIC_SLAVE_DATA;
        irq -= 8;
    }

    uint8_t curmask = inb(port);
    outb(port, curmask | (1 << irq));
}

void pic_mask_all()
{
    pic_outb(PORT_PIC_MASTER_DATA, 0xFF);
    pic_outb(PORT_PIC_SLAVE_DATA, 0xFF);
}

void pic_eoi(uint8_t irq)
{
    if(irq >= 0x8) //Если прерывание относится ко второму контроллеру (Slave), то необходимо сбросить оба
        outb(PORT_PIC_SLAVE_CMD, PIC_EOI);

    outb(PORT_PIC_MASTER_CMD, PIC_EOI);
    //lapic_eoi();
}