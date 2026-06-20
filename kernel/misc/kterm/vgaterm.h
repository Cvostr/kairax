#ifndef _VGATERM_H
#define _VGATERM_H

#include "types.h"

struct vgaconsole {
    uint32_t console_passed_chars;
    uint32_t console_lines;
    uint32_t console_col;

    uint8_t *double_buffer;
    uint32_t console_buffer_size;

    int autowrap;
};

struct vgaconsole* console_init();
void console_print_char(struct vgaconsole* vgconsole, int table, char chr,
    unsigned char r, unsigned char g, unsigned char b,
    unsigned char br, unsigned char bg, unsigned char bb);
void console_scroll(struct vgaconsole* vgconsole);
void console_backspace(struct vgaconsole* vgconsole, int chars);
void console_cr(struct vgaconsole* vgconsole);
void console_lf(struct vgaconsole* vgconsole);
void console_redraw(struct vgaconsole* vgconsole);
// полностью очистить экран
void console_clear(struct vgaconsole* vgconsole);
// очистить экран от курсора до конца
void console_clear_to_end(struct vgaconsole* vgconsole);
void console_set_cursor_pos(struct vgaconsole* vgconsole, int row, int col);
void console_cursor_move(struct vgaconsole* vgconsole, int dx, int dy);

void console_clear_line_to_end(struct vgaconsole* vgconsole);
void console_clear_line_to_begin(struct vgaconsole* vgconsole);

void console_del_chars(struct vgaconsole* vgconsole, uint32_t chars);

void surface_copy(struct vgaconsole* vgconsole, uint32_t x, uint32_t y, uint32_t nx, uint32_t ny, uint32_t width, uint32_t height);
void surface_draw_pixel(struct vgaconsole* vgconsole, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);
void surface_draw_rect(struct vgaconsole* vgconsole, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b);
void surface_draw_char(struct vgaconsole* vgconsole, uint8_t chartable[][8], char c, uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, int vscale, int hscale);

uint32 surface_get_rows();
uint32 surface_get_cols();

#endif