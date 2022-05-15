#ifndef _X86_CONSOLE
#define _X86_CONSOLE

void hide_console_cursor();

void show_console_cursor();

void set_cursor_pos(int cursorx, int cursory);

void clear_console();

void set_console_print_color(char color);

void print_char(char chr);

void print_string(const char* string);

#endif