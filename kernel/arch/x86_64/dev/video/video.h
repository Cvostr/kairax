#ifndef _VIDEO_H
#define _VIDEO_H

#include "types.h"

void vga_init(uint64_t addr, uint32_t pitch, uint32_t width, uint32_t height, uint32_t depth);

void vga_init_dev();

void vga_draw_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);

void vga_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b);

void vga_clear();

void console_print_char(char chr);

void console_remove_from_end(int chars);

#endif