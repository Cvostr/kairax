#ifndef _INT_KEYBOARD_H
#define _INT_KEYBOARD_H

#include "interrupts/handle/handler.h"

void init_ints_keyboard();

char keyboard_get_key();

char keyboard_get_key_ascii(char keycode);

void keyboard_int_handler(interrupt_frame_t* frame, void* data);

#endif
