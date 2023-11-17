#include "int_keyboard.h"
#include "interrupts/pic.h"
#include "io.h"
#include "fs/devfs/devfs.h"
#include "mem/kheap.h"
#include "keycodes.h"

struct file_operations int_keyb_fops;

struct keyboard_buffer* key_buffers[KEY_BUFFERS_MAX];
struct keyboard_buffer* main_key_buffer = NULL;

struct keyboard_buffer* new_keyboard_buffer() 
{
    for (int i = 0; i < KEY_BUFFERS_MAX; i ++) {
        if (key_buffers[i] == NULL) {

            // Создать буфер
            struct keyboard_buffer* kbuffer = kmalloc(sizeof(struct keyboard_buffer));
            memset(kbuffer, 0, sizeof(struct keyboard_buffer));

            key_buffers[i] = kbuffer;
            return kbuffer;
        }
    }

    return NULL;
}

ssize_t intk_f_read (struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct keyboard_buffer* keybuffer = (struct keyboard_buffer*) file->private_data;
    if (keybuffer->start == keybuffer->end) {
        return 0;
    }

    short pressed = keyboard_get_key_from_buffer(keybuffer);
    buffer[0] = pressed & 0xFF;
    buffer[1] = (pressed >> 8) & 0xFF;

    return 1;
}

int intk_f_open(struct inode *inode, struct file *file)
{
    struct keyboard_buffer* buffer = new_keyboard_buffer();
    file->private_data = buffer;
}

void init_ints_keyboard()
{
    memset(key_buffers, 0, sizeof(key_buffers));
    main_key_buffer = new_keyboard_buffer();

    pic_unmask(0x21);
    register_interrupt_handler(0x21, keyboard_int_handler, NULL);

    int_keyb_fops.read = intk_f_read;
    int_keyb_fops.open = intk_f_open;
	  devfs_add_char_device("keyboard", &int_keyb_fops);
}

void keyboard_int_handler(interrupt_frame_t* frame, void* data)
{
    char keycode_ps2 = inb(0x60);
    short keycode_krx = keycode_ps2_to_kairax(keycode_ps2);
    
    for (int i = 0; i < KEY_BUFFERS_MAX; i ++) {
        if (key_buffers[i] != NULL) {

            struct keyboard_buffer* buffer = key_buffers[i];

            if (buffer->end >= KEY_BUFFER_SIZE) 
                buffer->end = 0;

            buffer->buffer[buffer->end] = keycode_krx;
            buffer->end += 1;
        }
    }

    int stat61 = inb(0x61);
    outb(0x61, stat61 | 1);

    pic_eoi(1);
}

char keyboard_get_key()
{
    return keyboard_get_key_from_buffer(main_key_buffer);
}

short keyboard_get_key_from_buffer(struct keyboard_buffer* buffer)
{
    if (buffer->start != buffer->end) {
        if (buffer->start >= KEY_BUFFER_SIZE) buffer->start = 0;
            buffer->start++;
    } else {
        return 0;
    }

    return buffer->buffer[buffer->start - 1];
}

#define DEFINE_KEY(x, key) case 0x80 + x: \
                                state = KRXK_RELEASED; \
                            case x: \
                                keycode = key; \
                                break;

short keycode_ps2_to_kairax(unsigned char keycode_ps2)
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
        case 0x8E:
            state = KRXK_RELEASED;
        case 0x0E:
            keycode = KRXK_BKSP;
            break;
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
        //DEFINE_KEY(0x27, KRXK_) ;
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

    return (((short) state) << 8) | keycode;
}