#ifndef _KBD_H
#define _KBD_H

#include "kairax/types.h"

#define KEY_BUFFER_SIZE 20
#define KEY_BUFFERS_MAX 10

struct keyboard_buffer {
    short buffer[KEY_BUFFER_SIZE];
    unsigned char start;
    unsigned char end;
};

void keyboard_add_event(uint8_t key, uint8_t action);

short keyboard_get_key_from_buffer(struct keyboard_buffer* buffer);
short keyboard_get_key();

void keyboard_init();

#endif