#ifndef _INT_KEYBOARD_H
#define _INT_KEYBOARD_H

#include "interrupts/handle/handler.h"

void init_ints_keyboard();
uint8_t keycode_ps2_to_kairax(uint8_t keycode_ps2, uint8_t *pState);
void keyboard_int_handler(interrupt_frame_t* frame, void* data);

#endif
