#ifndef _VIDEO_H
#define _VIDEO_H

#include "types.h"
#include "fs/vfs/file.h"

void vga_init(void* addr, uint32_t pitch, uint32_t width, uint32_t height, uint32_t depth);
void vga_init_dev();

void vga_draw_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);
void vga_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b);
void vga_clear();

uint32_t vga_get_width();
uint32_t vga_get_height();
uint32_t vga_get_depth();
uint32_t vga_get_pitch();

ssize_t vga_display_write (struct file* file, const char* buffer, size_t count, loff_t offset);
void vga_draw(const char* buffer, uint32_t width, uint32_t height);

#endif