#include "int_keyboard.h"
#include "interrupts/pic.h"
#include "io.h"
#include "fs/devfs/devfs.h"

#define KEY_BUFFER_SIZE 16
char key_buffer[KEY_BUFFER_SIZE];
unsigned char buffer_start;
unsigned char buffer_end;

struct file_operations int_keyb_fops;

ssize_t intk_f_read (struct file* file, char* buffer, size_t count, loff_t offset)
{
    buffer[0] = keyboard_get_key();
    return buffer[0] != 0;
}

void init_ints_keyboard()
{
    buffer_start = 0;
    buffer_end = 0;

    pic_unmask(0x21);
    register_interrupt_handler(0x21, keyboard_int_handler, NULL);

    int_keyb_fops.read = intk_f_read;
	devfs_add_char_device("keyboard", &int_keyb_fops);
}

void keyboard_int_handler(interrupt_frame_t* frame, void* data)
{
    char keycode = inb(0x60) + 1;
    
    if (buffer_end >= KEY_BUFFER_SIZE) 
       buffer_end = 0;

    key_buffer[buffer_end] = keycode;
    buffer_end += 1;

    int stat61 = inb(0x61);
    outb(0x61, stat61 | 1);

    pic_eoi(1);
}

char keyboard_get_key()
{
    if (buffer_start != buffer_end) {
        if(buffer_start >= KEY_BUFFER_SIZE) buffer_start = 0;
            buffer_start++;
    } else {
        return 0;
    }

    return key_buffer[buffer_start - 1];
}

char keyboard_get_key_ascii(char keycode)
{
	switch(keycode) {
    case 0x11:
        return 'q';
    case 0x12:
        return 'w';
    case 0x13:
      return 'e';
      break;
    case 0x14:
      return 'r';
      break;
    case 0x15:
      return 't';
      break;
    case 0x16:
      return 'y';
      break;
    case 0x17:
      return 'u';
      break;
    case 0x18:
      return 'i';
      break;
    case 0x19:
      return 'o';
      break;
    case 0x1A:
      return 'p';
      break;
    case 0x1F:
      return 'a';
      break;
    case 0x20:
      return 's';
      break;
    case 0x21:
      return 'd';
      break;
    case 0x22:
      return 'f';
      break;
    case 0x23:
      return 'g';
      break;
    case 0x24:
      return 'h';
      break;
    case 0x25:
      return 'j';
      break;
    case 0x26:
      return 'k';
      break;
    case 0x27:
      return 'l';
      break;
    case 0x2D:
      return 'z';
      break;
    case 0x2E:
      return 'x';
      break;
    case 0x2F:
      return 'c';
      break;
    case 0x30:
      return 'v';
      break;
    case 0x31:
      return 'b';
      break;
    case 0x32:
      return 'n';
      break;
    case 0x33:
      return 'm';
      break;
    case 0x3:
      return '1';
      break;
    case 0x4:
      return '2';
      break;
    case 0x5:
      return '3';
      break;
    case 0x6:
      return '4';
      break;
    case 0x7:
      return '5';
      break;
    case 0x8:
      return '6';
      break;
    case 0x9:
      return '7';
      break;
    case 0xA:
      return '8';
      break;
    case 0xB:
      return '9';
      break;
    case 0xC:
      return '0';
      break;
    case 0xD:
        return '-';
    case 0x1D:
        return '\n';
    case 0x3A:
      return ' ';
      break;
    case 0x36:
      return '/';
      break;
    case 0x35:
      return '.';
      break;
    default:
        //if (keycode != 0)
        //printf("%i ", keycode);
	}
	return 0;
}