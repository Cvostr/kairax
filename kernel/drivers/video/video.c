#include "video.h"
#include "memory/mem_layout.h"
#include "types.h"
#include "string.h"
#include "io.h"
#include "fs/devfs/devfs.h"
#include "mem/kheap.h"
#include "sync/spinlock.h"
#include "mem/pmm.h"

char* _addr;
uint32_t _pitch;
uint32_t _width;
uint32_t _height;
uint32_t _depth;

uint8_t* fb_double_buffer = NULL;

struct file_operations vga_disp_fops;

void vga_init(void* addr, uint32_t pitch, uint32_t width, uint32_t height, uint32_t depth)
{
    _addr = addr;
    _pitch = pitch;
    _width = width;
    _height = height;
    _depth = depth;

    size_t vga_buffer_size = (_width * _height * _depth) / 8;
    memset(_addr, 0, vga_buffer_size);

    fb_double_buffer = kmalloc(vga_buffer_size);
    fb_double_buffer = P2V(vga_buffer_size);
    memset(fb_double_buffer, 0, vga_buffer_size);
}

uint32_t vga_get_width()
{
    return _width;
}
uint32_t vga_get_height()
{
    return _height;
}
uint32_t vga_get_depth()
{
    return _depth;
}
uint32_t vga_get_pitch()
{
    return _pitch;
}

void vga_init_dev()
{
    vga_disp_fops.write = vga_display_write;
	devfs_add_char_device("display", &vga_disp_fops, NULL);
}

void vga_draw_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) 
{
    int pixwidth = _depth / 8;
    // Запись в экранный буфер
    char* dfb_addr = fb_double_buffer + y * _pitch + x * pixwidth;
    
    if (dfb_addr[0] != b || dfb_addr[1] != g || dfb_addr[2] != r) {
        dfb_addr[0] = b;
        dfb_addr[1] = g;
        dfb_addr[2] = r;

        char* fb_addr = _addr + y * _pitch + x * pixwidth;
        *((uint32_t*) fb_addr) = *((uint32_t*) dfb_addr);
    }
}

void vga_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b) 
{
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            vga_draw_pixel(j + x, i + y, r, g, b);
        }
    }
}

void vga_clear() 
{
    memset(_addr, 0, _pitch * _height);
}

ssize_t vga_display_write (struct file* file, const char* buffer, size_t count, loff_t offset)
{
    int width = *((int*) buffer);
    buffer += 4;
    int height = *((int*) buffer);
    buffer += 4;

    vga_draw(buffer, width, height);

    return count;
}

void vga_draw(const char* buffer, uint32_t width, uint32_t height)
{
    for (int i = 0; i < width; i ++) {
        for (int j = 0; j < height; j ++) {
            vga_draw_pixel(j, i, buffer[0], buffer[1], buffer[2]);
            buffer += 4;
        }
    }
}