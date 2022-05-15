#include "int_keyboard.h"
#include "interrupts/pic.h"
#include "interrupts/handle/handler.h"
#include "io.h"
#include "x86-console/x86-console.h"

void keyboard_int_handler(interrupt_frame_t* frame);

void init_ints_keyboard(){
	pic_unmask(0x21);
	register_interrupt_handler(0x21, keyboard_int_handler);
}

void keyboard_int_handler(interrupt_frame_t* frame){
	char keycode = inb(0x60) + 1;
	print_char(keycode);
	//if(buffer_end >= KEY_BUFFER_SIZE) buffer_end = 0;

	//key_buffer[buffer_end] = keycode;
	//buffer_end += 1;

	int stat61 = inb(0x61);
	outb(0x61, stat61 | 1);

	pic_eoi(1);
}