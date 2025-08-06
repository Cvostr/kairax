#include "ps2.h"
#include "io.h"

void ps2_mouse_setup(int portid)
{
    printk("PS/2: Setting up mouse on port %i\n", portid);
}

void ps2_mouse_irq_handler()
{
    uint8_t dummy = inb(PS2_DATA_REG);
    //printk("Mouse");
}