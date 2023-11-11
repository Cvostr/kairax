#ifndef _VGATERM_H
#define _VGATERM_H

#include "types.h"

void console_init();
void console_print_char(char chr);
void console_scroll();
void console_backspace(int chars);
void console_cr();
void console_lf();

void surface_draw_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);
void surface_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b);
void surface_draw_char(char c, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, int vscale, int hscale);

#endif