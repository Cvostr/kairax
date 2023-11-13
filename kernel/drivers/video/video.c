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

struct file_operations vga_disp_fops;

void vga_init(void* addr, uint32_t pitch, uint32_t width, uint32_t height, uint32_t depth)
{
    _addr = addr;
    _pitch = pitch;
    _width = width;
    _height = height;
    _depth = depth;

    //printf("width %i, height %i, addr %i\n", 
    //    kboot_info->fb_info.fb_width,
    //    kboot_info->fb_info.fb_height,
    //    kboot_info->fb_info.fb_addr);
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
	devfs_add_char_device("display", &vga_disp_fops);
}

void vga_draw_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) 
{
    int pixwidth = _depth / 8;
    // Запись в экранный буфер
    char* fb_addr = _addr + y * _pitch + x * pixwidth;
    fb_addr[0] = b;
    fb_addr[1] = g;
    fb_addr[2] = r;
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
            vga_draw_pixel(j, i, buffer[2], buffer[1], buffer[0]);
            buffer += 4;
        }
    }
}