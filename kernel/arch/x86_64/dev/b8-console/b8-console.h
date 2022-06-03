#ifndef _B8_CONSOLE
#define _B8_CONSOLE

void b8_console_hide_cursor();

void b8_console_show_cursor();

void b8_console_set_cursor_pos(int cursorx, int cursory);

void b8_console_clear();

void b8_console_set_print_color(char color);

void b8_console_print_char(char chr);

void b8_console_print_string(const char* string);

void b8_remove_from_end(int chars);

#endif