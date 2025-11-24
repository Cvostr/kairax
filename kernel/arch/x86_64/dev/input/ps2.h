#ifndef _INT_KEYBOARD_H
#define _INT_KEYBOARD_H

#include "interrupts/handle/handler.h"

#define PS2_DATA_REG    0x60
#define PS2_STATUS_REG  0x64
#define PS2_CMD_REG     0x64

#define PS2_READ_CONFIG     0x20
#define PS2_WRITE_CONFIG    0x60
#define PS2_TEST_CONTROLLER 0xAA
#define PS2_TEST_PORT1      0xAB
#define PS2_DISABLE_PORT1   0xAD
#define PS2_ENABLE_PORT1    0xAE
#define PS2_WRITE_TO_PORT2  0xD4
#define PS2_DISABLE_PORT2   0xA7
#define PS2_ENABLE_PORT2    0xA8
#define PS2_TEST_PORT2      0xA9

#define PS2_IRQ_PORT1       (1 << 0)
#define PS2_IRQ_PORT2       (1 << 1)
#define PS2_CLOCK_PORT1     (1 << 4)
#define	PS2_CLOCK_PORT2     (1 << 5)
#define	PS2_TRANSLATION_PORT1  (1 << 6)

#define PS2_TEST_PORT1_PASS         0x00
#define PS2_TEST_PORT2_PASS         0x00
#define PS2_TEST_CONTROLLER_PASS    0x55
#define PS2_SELF_TEST_PASS          0xAA
#define PS2_ACK                     0xFA
#define PS2_RESEND                  0xFE

#define PS2_ENABLE_SCANNING     0xF4
#define PS2_DISABLE_SCANNING    0xF5
#define PS2_IDENTIFY            0xF2
#define PS2_RESET               0xFF

#define PS2_PORT1_IRQ_NUM   1
#define PS2_PORT2_IRQ_NUM   12

#define PS2_KEYBOARD        1
#define PS2_MOUSE           2

int ps2_wait_input();
int ps2_cmd(uint8_t cmd);
void ps2_cmd1(uint8_t cmd, uint8_t arg);
int ps2_read_byte(uint8_t* out);

int ps2_perform_controller_test();
int ps2_perform_interface_test(uint8_t cmd);
int ps2_has_port2();

void init_ps2();
uint8_t keycode_ps2_to_kairax(uint8_t keycode_ps2, uint8_t *pState);
void ps2_device_setup(int portid, int type);

int ps2_write_byte_and_wait(int portid, uint8_t data);

// Клавиатура
void ps2_kbd_irq_handler();
void ps2_kbd_setup(int portid);

// Мышь
void ps2_mouse_irq_handler();
void ps2_mouse_setup(int portid);

#endif
