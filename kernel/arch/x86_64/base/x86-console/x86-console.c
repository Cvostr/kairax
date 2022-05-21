#include "memory/hh_offset.h"
#include "x86-console.h"
#include "stdint.h"
#include "string.h"
#include "io.h"

#define BUFFER_SIZE 4000 	//80 * 25 * 2
#define BUFFER_LINE_SIZE 160	//80 * 2

uint32_t passed_chars = 0;
char text_printing_color = 0x07;

char text_buffer[BUFFER_SIZE]; //4000 chars
int overstep = 0;

volatile char* getTextVmemPtr(){
	//return (volatile char*)(P2V(0xB8000));
	return (volatile char*)(0xB8000);
}

void set_console_print_color(char color){
	text_printing_color = color;
}

void hide_console_cursor(){
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void show_console_cursor(){
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3E0) & 0xE0) | 1);
}

void set_cursor_pos(int cursorx, int cursory){
	uint16_t pos = cursory * 80 + cursorx;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void clear_console(){
	volatile char* vidptr = getTextVmemPtr();
	int j = 0;
  	while(j < BUFFER_SIZE){
    		vidptr[j] = ' ';
		vidptr[j+1] = text_printing_color;
		j = j + 2;
  	}
  	passed_chars = 0;
}

void checkForRefill(){
	if(passed_chars >= BUFFER_SIZE){
		strncpy(&(text_buffer[0]), (char*)getTextVmemPtr() + BUFFER_LINE_SIZE, BUFFER_SIZE - BUFFER_LINE_SIZE);
		clear_console();
		strncpy((char*)getTextVmemPtr(), &(text_buffer[overstep]), BUFFER_SIZE - BUFFER_LINE_SIZE);
		passed_chars = BUFFER_SIZE - BUFFER_LINE_SIZE;
	}
}

void print_char(char chr){
	volatile char* step = getTextVmemPtr() + passed_chars; //We'll move this pointer
	if(chr != '\n'){
		*step = chr; //Write to screen buffer
		step++; //Move pointer
		*step = text_printing_color;
		passed_chars += 2;
		checkForRefill();
	}else{
		passed_chars += (BUFFER_LINE_SIZE - (passed_chars % BUFFER_LINE_SIZE)) - 2; //-2 means we avoid space
		*step = ' ';
		passed_chars += 2;
		checkForRefill();
	}
}

void print_string(const char* string){
	char* _str = (char*)string;
	while(_str[0] != '\0'){
		print_char(_str[0]);
		_str++;
	}
}