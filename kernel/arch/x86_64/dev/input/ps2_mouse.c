#include "ps2.h"
#include "io.h"
#include "kairax/types.h"

#define PS2_MOUSE_SET_SAMPLE_RATE   0xF3

uint8_t ps2_mouse_buffer[4];
uint8_t ps2_mouse_buffer_pos = 0;
uint8_t ps2_mouse_buffer_len = 3;

// extension
int ps2_mouse_z_axis = 0;

/*
yo	Y-Axis Overflow
xo	X-Axis Overflow
ys	Y-Axis Sign Bit (9-Bit Y-Axis Relative Offset)
xs	X-Axis Sign Bit (9-Bit X-Axis Relative Offset)
1	Always One
bm	Button Middle (Normally Off = 0)
br	Button Right (Normally Off = 0)
bl	Button Left (Normally Off = 0)
*/

struct ps2_mouse {
    uint8_t desc;       // yo xo ys xs 1 bm br bl
    uint8_t axis_x;
    uint8_t axis_y;
    uint8_t axis_z;
};

void ps2_mouse_setup(int portid)
{
    printk("PS/2: Setting up mouse on port %i\n", portid);

    uint8_t out = 0;
    int ident_len = 0;
    uint8_t ident[2];

    // Пробуем включить расширение с колёсиком
    // Для этого надо выполнить такую вот магическую последовательность
    ps2_write_byte_and_wait(portid, PS2_MOUSE_SET_SAMPLE_RATE);
    ps2_write_byte_and_wait(portid, 200);
    ps2_write_byte_and_wait(portid, PS2_MOUSE_SET_SAMPLE_RATE);
    ps2_write_byte_and_wait(portid, 100);
    ps2_write_byte_and_wait(portid, PS2_MOUSE_SET_SAMPLE_RATE);
    ps2_write_byte_and_wait(portid, 80);

    // Снова выполним IDENTIFY
    int rc = ps2_write_byte_and_wait(portid, PS2_IDENTIFY);
    if (rc != 0)
    {
        printk("PS2: IDENTIFY error %i on port %i\n", rc, portid);
        return;
    }

    // Считаем identify
    for (int i = 0; i < 2; i ++)
    {
        out = 0;
        rc = ps2_read_byte(&out);
        if (rc == FALSE)
        {
            break;
        }

        ident[ident_len++] = out;
    }

    if (ident_len == 0)
    {
        printk("PS2: Ident failed\n");
        return;
    }

    // Первый байт - MouseID
    uint8_t mouse_id = ident[0];
    if (mouse_id == 3)
    {
        // Если mouseID стал 3, то мышь поддерживает колёсико
        ps2_mouse_z_axis = TRUE;
        // Размер буфера теперь 4 байт
        ps2_mouse_buffer_len = 4;
        // TODO: 5 buttons extension??
    } 
    else if  (mouse_id == 0)
    {
        printk("PS2 Mouse does not support wheel\n");
    } 
    else
    {
        printk("PS2 Mouse unknown mouseId %x\n", mouse_id);
    }
}

void ps2_mouse_irq_handler()
{
    // Считать
    uint8_t mouse_packet = inb(PS2_DATA_REG);
    // Поместить в буфер
    ps2_mouse_buffer[ps2_mouse_buffer_pos++] = mouse_packet;
    // По спецификации, у первого байта бит 3 должен быть 1. Проверим это
    if ((ps2_mouse_buffer[0] & 0x08) == 0)
    {
        printk("PS2 Mouse: Corrupted packet 0\n");
        return;
    }

    if (ps2_mouse_buffer_pos == ps2_mouse_buffer_len)
    {
        // Считали один полный буфер
        // Сбрасываем позицию
        ps2_mouse_buffer_pos = 0;
        //printk("Mouse %x %x %x %x\n", ps2_mouse_buffer[0], ps2_mouse_buffer[1], ps2_mouse_buffer[2]);
        // https://wiki.osdev.org/PS/2_Mouse
    }
}