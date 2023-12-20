#ifndef _VGATERM_H
#define _VGATERM_H

#include "types.h"

struct vgaconsole {
    uint32_t console_passed_chars;
    uint32_t console_lines;
    uint32_t console_col;

    char* double_buffer;
    uint32_t console_buffer_size;
};

struct vgaconsole* console_init();
void console_print_char(struct vgaconsole* vgconsole, char chr, unsigned char r, unsigned char g, unsigned char b);
void console_scroll(struct vgaconsole* vgconsole);
void console_backspace(struct vgaconsole* vgconsole, int chars);
void console_cr(struct vgaconsole* vgconsole);
void console_lf(struct vgaconsole* vgconsole);
void console_redraw(struct vgaconsole* vgconsole);

void surface_draw_pixel(struct vgaconsole* vgconsole, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);
void surface_draw_rect(struct vgaconsole* vgconsole, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b);
void surface_draw_char(struct vgaconsole* vgconsole, char c, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, int vscale, int hscale);

#endif