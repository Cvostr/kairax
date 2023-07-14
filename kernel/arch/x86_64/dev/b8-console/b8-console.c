#include "memory/mem_layout.h"
#include "b8-console.h"
#include "types.h"
#include "string.h"
#include "io.h"

#define BUFFER_SIZE 4000 	//80 * 25 * 2
#define BUFFER_LINE_SIZE 160	//80 * 2

uint32_t passed_chars = 0;
char text_printing_color = 0x07;

char text_buffer[BUFFER_SIZE]; //4000 chars
int overstep = 0;

void* b8_addr = (char*)0xB8000;

char* b8_get_text_addr(){
	return b8_addr;
}

void b8_set_addr(void* addr) 
{
	b8_addr = addr;
}

void b8_console_set_print_color(char color){
	text_printing_color = color;
}

void b8_console_hide_cursor(){
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void b8_console_show_cursor(){
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3E0) & 0xE0) | 1);
}

void b8_console_set_cursor_pos(int cursorx, int cursory){
	uint16_t pos = cursory * 80 + cursorx;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void b8_console_clear(){
	char* vidptr = b8_get_text_addr();
	int j = 0;
  	while(j < BUFFER_SIZE){
    		vidptr[j] = ' ';
		vidptr[j+1] = text_printing_color;
		j = j + 2;
  	}
  	passed_chars = 0;
}

void b8_check_for_refill()
{
	if(passed_chars >= BUFFER_SIZE){
		strncpy(text_buffer, (char*)b8_get_text_addr() + BUFFER_LINE_SIZE, BUFFER_SIZE - BUFFER_LINE_SIZE);
		b8_console_clear();
		strncpy((char*)b8_get_text_addr(), text_buffer, BUFFER_SIZE - BUFFER_LINE_SIZE);
		passed_chars = BUFFER_SIZE - BUFFER_LINE_SIZE;
	}
}

void b8_console_print_char(char chr)
{
	char* step = b8_get_text_addr() + passed_chars;
	if (chr != '\n') {
		*step = chr; //Записать в буфер
		step++; //Сдвинуть указатеь на 1 байт
		*step = text_printing_color;
		passed_chars += 2;
		b8_check_for_refill();
	} else {
		passed_chars += (BUFFER_LINE_SIZE - (passed_chars % BUFFER_LINE_SIZE)) - 2;
		*step = ' ';
		passed_chars += 2;
		b8_check_for_refill();
	}
}

void b8_console_print_string(const char* string){
	char* _str = (char*)string;
	while(_str[0] != '\0'){
		b8_console_print_char(_str[0]);
		_str++;
	}
}

void b8_remove_from_end(int chars){
	for(int i = 0; i < chars; i ++){
		passed_chars -= 2;
		volatile char* step = b8_get_text_addr() + passed_chars;
		*step = ' ';
	}
}