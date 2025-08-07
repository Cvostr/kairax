#include "xt_kbd.h"
#include "interrupts/pic.h"
#include "io.h"
#include "mem/kheap.h"
#include "keycodes.h"
#include "sync/spinlock.h"
#include "string.h"
#include "dev/interrupts.h"
#include "interrupts/ioapic.h"
#include "drivers/char/input/keyboard.h"

int xt_tag_E0 = 0;

void init_xt_keyboard()
{
    register_irq_handler(1, xt_kbd_irq_handler, NULL);
}

void xt_kbd_irq_handler(interrupt_frame_t* frame, void* data)
{
    uint8_t keycode_ps2 = inb(0x60);
    if (keycode_ps2 != 0xE0) 
    {
        uint8_t action = 0;
        uint8_t keycode_krx = kbd_codeset1_to_kairax(keycode_ps2, &action);
        
        keyboard_add_event(keycode_krx, action);

        xt_tag_E0 = 0;
    } else {
        xt_tag_E0 = 1;
    }

    int stat61 = inb(0x61);
    outb(0x61, stat61 | 1);
}

#define DEFINE_KEY(x, key) case 0x80 + x: \
                                state = KRXK_RELEASED; \
                            case x: \
                                keycode = key; \
                                break;

uint8_t kbd_codeset1_to_kairax(uint8_t keycode_ps2, uint8_t *pState)
{
    char state = 0;
    char keycode;
    switch (keycode_ps2) {
        case 0x81:          // ESCAPE
            state = KRXK_RELEASED;
        case 0x01:
            keycode = KRXK_ESCAPE;
            break;
        case 0x82:          // 1
            state = KRXK_RELEASED;
        case 0x02:
            keycode = KRXK_1;
            break;
        case 0x83:          // 2
            state = KRXK_RELEASED;
        case 0x03:   
            keycode = KRXK_2;
            break;
        case 0x84:
            state = KRXK_RELEASED;
        case 0x04:
            keycode = KRXK_3;
            break;
        case 0x85:
            state = KRXK_RELEASED;
        case 0x05:
            keycode = KRXK_4;
            break;
        case 0x86:
            state = KRXK_RELEASED;
        case 0x06:
            keycode = KRXK_5;
            break;
        case 0x87:
            state = KRXK_RELEASED;
        case 0x07:
            keycode = KRXK_6;
            break;
        case 0x88:
            state = KRXK_RELEASED;
        case 0x08:
            keycode = KRXK_7;
            break;
        case 0x89:
            state = KRXK_RELEASED;
        case 0x09:
            keycode = KRXK_8;
            break;
        case 0x8A:
            state = KRXK_RELEASED;
        case 0x0A:
            keycode = KRXK_9;
            break;
        case 0x8B:
            state = KRXK_RELEASED;
        case 0x0B:
            keycode = KRXK_0;
            break;
        case 0x8C:
            state = KRXK_RELEASED;
        case 0x0C:
            keycode = KRXK_MINUS;
            break;
        DEFINE_KEY(0x0D, KRXK_EQUAL)
        DEFINE_KEY(0x0E, KRXK_BKSP)
        DEFINE_KEY(0x0F, KRXK_TAB)
        DEFINE_KEY(0x10, KRXK_Q)       
        DEFINE_KEY(0x11, KRXK_W)
        DEFINE_KEY(0x12, KRXK_E)
        DEFINE_KEY(0x13, KRXK_R)
        DEFINE_KEY(0x14, KRXK_T)
        DEFINE_KEY(0x15, KRXK_Y)
        DEFINE_KEY(0x16, KRXK_U)
        DEFINE_KEY(0x17, KRXK_I)
        DEFINE_KEY(0x18, KRXK_O)
        DEFINE_KEY(0x19, KRXK_P)
        //DEFINE_KEY(0x1A, KRXK_TAB)    [
        //DEFINE_KEY(0x1B, KRXK_TAB)    ]
        DEFINE_KEY(0x1C, KRXK_ENTER)
        DEFINE_KEY(0x1D, KRXK_LCTRL)
        DEFINE_KEY(0x1E, KRXK_A)
        DEFINE_KEY(0x1F, KRXK_S)
        DEFINE_KEY(0x20, KRXK_D)
        DEFINE_KEY(0x21, KRXK_F)
        DEFINE_KEY(0x22, KRXK_G)
        DEFINE_KEY(0x23, KRXK_H)
        DEFINE_KEY(0x24, KRXK_J)
        DEFINE_KEY(0x25, KRXK_K)
        DEFINE_KEY(0x26, KRXK_L)
        DEFINE_KEY(0x27, KRXK_SEMICOLON)
        DEFINE_KEY(0x28, KRXK_QUOTES)
        DEFINE_KEY(0x29, KRXK_BACK_TICK)
        DEFINE_KEY(0x2A, KRXK_LSHIFT)
        DEFINE_KEY(0x2B, KRXK_BSLASH)
        DEFINE_KEY(0x2C, KRXK_Z)
        DEFINE_KEY(0x2D, KRXK_X)
        DEFINE_KEY(0x2E, KRXK_C)
        DEFINE_KEY(0x2F, KRXK_V)
        DEFINE_KEY(0x30, KRXK_B)
        DEFINE_KEY(0x31, KRXK_N)
        DEFINE_KEY(0x32, KRXK_M)
        DEFINE_KEY(0x33, KRXK_COMMA)
        DEFINE_KEY(0x34, KRXK_DOT)
        DEFINE_KEY(0x35, KRXK_SLASH)
        DEFINE_KEY(0x36, KRXK_RSHIFT)
        DEFINE_KEY(0x38, KRXK_LALT)
        DEFINE_KEY(0x39, KRXK_SPACE)
        DEFINE_KEY(0x3A, KRXK_CAPS)
        DEFINE_KEY(0x3B, KRXK_F1)
        DEFINE_KEY(0x3C, KRXK_F2)
        DEFINE_KEY(0x3D, KRXK_F3)
        DEFINE_KEY(0x3E, KRXK_F4)
        DEFINE_KEY(0x3F, KRXK_F5)
        DEFINE_KEY(0x40, KRXK_F6)
        DEFINE_KEY(0x41, KRXK_F7)
        DEFINE_KEY(0x42, KRXK_F8)
        DEFINE_KEY(0x43, KRXK_F9)
        DEFINE_KEY(0x44, KRXK_F10)
        DEFINE_KEY(0x45, KRXK_NUMLOCK)
        DEFINE_KEY(0x46, KRXK_SCRLOCK)

        DEFINE_KEY(0x57, KRXK_F11)
        DEFINE_KEY(0x58, KRXK_F12)
    }

    *pState = state;

    return keycode;
}