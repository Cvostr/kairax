#ifndef _INT_KEYBOARD_H
#define _INT_KEYBOARD_H

#include "interrupts/handle/handler.h"

void init_xt_keyboard();
uint8_t kbd_codeset1_to_kairax(uint8_t keycode_ps2, uint8_t *pState);
void xt_kbd_irq_handler(interrupt_frame_t* frame, void* data);

#endif
