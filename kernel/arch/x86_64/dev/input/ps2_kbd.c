#include "ps2.h"
#include "drivers/char/input/keyboard.h"
#include "keycodes.h"
#include "io.h"
#include "kairax/stdio.h"

// PS2 Keyboard команды
#define PS2_KBD_SET_LEDS            0xED
#define PS2_KBD_SET_SCAN_CODE_SET   0xF0

int ps2kbd_tag_E0 = 0;
int ps2kbd_tag_F0 = 0;

uint8_t kbd_set2_ext_mappings[256] = {
    [0x11] = KRXK_RALT,
    [0x6B] = KRXK_LEFT,
    [0x71] = KRXK_DEL,
    [0x72] = KRXK_DOWN,
    [0x74] = KRXK_RIGHT,
    [0x75] = KRXK_UP,
};

uint8_t kbd_set2_mappings[256] = {
    [0x01] = KRXK_F9,
    [0x03] = KRXK_F5,
    [0x04] = KRXK_F3,
    [0x05] = KRXK_F1, 
    [0x06] = KRXK_F2, 
    [0x07] = KRXK_F12, 
    [0x09] = KRXK_F10,
    [0x0A] = KRXK_F8,
    [0x0B] = KRXK_F6,
    [0x0C] = KRXK_F4,
    [0x0D] = KRXK_TAB,   
    [0x0E] = KRXK_BACK_TICK,
    [0x11] = KRXK_LALT,
    [0x12] = KRXK_LSHIFT,
    [0x14] = KRXK_LCTRL,
    [0x15] = KRXK_Q,     
    [0x16] = KRXK_1,
    [0x1A] = KRXK_Z,
    [0x1B] = KRXK_S,
    [0x1C] = KRXK_A,
    [0x1D] = KRXK_W,
    [0x1E] = KRXK_2,
    [0x21] = KRXK_C,
    [0x22] = KRXK_X,
    [0x23] = KRXK_D,
    [0x24] = KRXK_E,
    [0x25] = KRXK_4,
    [0x26] = KRXK_3,
    [0x29] = KRXK_SPACE,
    [0x2A] = KRXK_V,
    [0x2B] = KRXK_F,
    [0x2C] = KRXK_T,
    [0x2D] = KRXK_R,
    [0x2E] = KRXK_5,
    [0x31] = KRXK_N,
    [0x32] = KRXK_B,
    [0x33] = KRXK_H,
    [0x34] = KRXK_G,
    [0x35] = KRXK_Y,
    [0x36] = KRXK_6,
    [0x3A] = KRXK_M,
    [0x3B] = KRXK_J,
    [0x3C] = KRXK_U,
    [0x3D] = KRXK_7,
    [0x3E] = KRXK_8,
    [0x41] = KRXK_COMMA,
    [0x42] = KRXK_K,
    [0x43] = KRXK_I,
    [0x44] = KRXK_O,
    [0x45] = KRXK_0,
    [0x46] = KRXK_9,
    [0x49] = KRXK_DOT,
    [0x4A] = KRXK_SLASH,
    [0x4B] = KRXK_L,
    [0x4C] = KRXK_SEMICOLON,
    [0x4D] = KRXK_P,
    [0x4E] = KRXK_MINUS,
    [0x52] = KRXK_QUOTES,
    //[0x54] = KRXK_P,  [
    [0x55] = KRXK_EQUAL,
    [0x58] = KRXK_CAPS,
    [0x59] = KRXK_RSHIFT,
    [0x5A] = KRXK_ENTER,
    //[0x5B] = KRXK_ENTER,  ]
    [0x5D] = KRXK_BSLASH,
    [0x66] = KRXK_BKSP,

    [0x69] = KRXK_END,

    [0x70] = KRXK_INSERT,

    [0x76] = KRXK_ESCAPE,
    [0x77] = KRXK_NUMLOCK,

    [0x78] = KRXK_F11,

    [0x7E] = KRXK_SCRLOCK,

    [0x83] = KRXK_F7

};

void ps2_kbd_setup(int portid)
{
    printk("PS/2: Setting up keyboard on port %i\n", portid);

    ps2_write_byte_and_wait(portid, PS2_KBD_SET_LEDS);
    ps2_write_byte_and_wait(portid, 0);

    ps2_write_byte_and_wait(portid, PS2_KBD_SET_SCAN_CODE_SET);
    ps2_write_byte_and_wait(portid, 2);
}

void ps2_kbd_irq_handler()
{
    uint8_t keycode_ps2 = inb(PS2_DATA_REG);

    if (keycode_ps2 == 0xE0)
        ps2kbd_tag_E0 = 1;
    else if (keycode_ps2 == 0xF0)
    {
        ps2kbd_tag_F0 = 1;
    }
    else
    {
        uint8_t action = ps2kbd_tag_F0 == 0 ? KRXK_PRESSED : KRXK_RELEASED;
        uint8_t* keymap = ps2kbd_tag_E0 == 0 ? kbd_set2_mappings : kbd_set2_ext_mappings;

        uint8_t keycode_krx = keymap[keycode_ps2];
    
        // Добавить в очередь
        keyboard_add_event(keycode_krx, action);

        // Сбросить флаги
        ps2kbd_tag_E0 = 0;
        ps2kbd_tag_F0 = 0;
    }
}