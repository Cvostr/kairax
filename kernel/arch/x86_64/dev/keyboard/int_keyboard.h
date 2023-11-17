#ifndef _INT_KEYBOARD_H
#define _INT_KEYBOARD_H

#include "interrupts/handle/handler.h"

#define KEY_BUFFER_SIZE 16
#define KEY_BUFFERS_MAX 10

struct keyboard_buffer {
    short buffer[KEY_BUFFER_SIZE];
    unsigned char start;
    unsigned char end;
};

void init_ints_keyboard();

short keyboard_get_key();
short keyboard_get_key_from_buffer(struct keyboard_buffer* buffer);

char keyboard_get_key_ascii(char keycode);
short keycode_ps2_to_kairax(unsigned char keycode_ps2);

void keyboard_int_handler(interrupt_frame_t* frame, void* data);

#endif
