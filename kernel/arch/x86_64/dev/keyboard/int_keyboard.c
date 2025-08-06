#include "int_keyboard.h"
#include "interrupts/pic.h"
#include "io.h"
#include "mem/kheap.h"
#include "keycodes.h"
#include "sync/spinlock.h"
#include "string.h"
#include "dev/interrupts.h"
#include "interrupts/ioapic.h"
#include "drivers/char/input/keyboard.h"
#include "dev/acpi/acpi.h"
#include "kairax/stdio.h"

int ps2kbd_tag_E0 = 0;
int ps2kbd_tag_F0 = 0;

int ps2_wait_input()
{
	uint64_t timeout = 100000UL;
	while (--timeout) 
    {
		if (!(inb(PS2_STATUS_REG) & (1 << 1))) 
            return FALSE;
	}
    
	return TRUE;
}

int ps2_cmd(uint8_t cmd)
{
	int rc = ps2_wait_input();
	outb(PS2_CMD_REG, cmd);

    return 0;
}

void ps2_cmd1(uint8_t cmd, uint8_t arg)
{
	int rc = ps2_wait_input();
	outb(PS2_CMD_REG, cmd);

    rc = ps2_wait_input();
	outb(PS2_DATA_REG, arg);
}

int ps2_read_byte(uint8_t* out)
{
    uint64_t timeout = 100000UL;
	while (--timeout) 
    {
		if (!(inb(PS2_STATUS_REG) & 1)) 
            continue;

        *out = inb(PS2_DATA_REG);
        return TRUE;
	}
    
	return FALSE;
}

int ps2_perform_controller_test()
{
    ps2_cmd(PS2_TEST_CONTROLLER);
    uint8_t test_result;
    ps2_read_byte(&test_result);
    if (test_result != PS2_TEST_CONTROLLER_PASS)
    {
        return FALSE;
    }

    return TRUE;
}

int ps2_perform_interface_test(uint8_t cmd)
{
    ps2_cmd(cmd);
    uint8_t test_result;
    ps2_read_byte(&test_result);
    if (test_result != 0)
    {
        return FALSE;
    }

    return TRUE;
}

int ps2_has_port2()
{
    int result = 0;
    // Пытаемся включить порт 2
    ps2_cmd(PS2_ENABLE_PORT2);

    // Считаем байт конфигурации
    uint8_t config;
	ps2_cmd(PS2_READ_CONFIG);
	ps2_read_byte(&config);
    
    if (!(config & PS2_CLOCK_PORT2))
    {
        result = 1;
    }

    // Обратно выключаем порт
    ps2_cmd(PS2_DISABLE_PORT2);

    return result;
}

int ps2_write_byte_to_port(int portid, uint8_t data)
{
    if (portid == 1)
    {
        // Для второго порта
        ps2_cmd(PS2_WRITE_TO_PORT2);
    }

    ps2_wait_input();
	outb(PS2_DATA_REG, data);

    return 0;
}

int ps2_write_byte_and_wait(int portid, uint8_t data)
{
    uint8_t resp = 0;
    int rc;
    for (int i = 0; i < 10; i++)
	{
        ps2_write_byte_to_port(portid, data);

        rc = ps2_read_byte(&resp);
        if (rc != TRUE)
        {
            printk("ps2_read_byte() failed\n");
            return -3;
        }

        if (resp == PS2_RESEND)
        {
            continue;
        }

        if (resp == PS2_ACK)
        {
            return 0;
        }

        printk("PS/2: Incorrect response %i\n", resp);
        return -1;
    }

    printk("PS/2: Timeout\n");
    return -2;
}

int ps2_reset_and_identity(int portid, int* type)
{
    uint8_t out = 0;
    uint8_t dummy;
    int rc;
    // Сброс устройства и self тест
    ps2_write_byte_and_wait(portid, PS2_RESET);
    ps2_read_byte(&out);
    if (out != PS2_SELF_TEST_PASS)
    {
        return -1;
    }
    // Очистка Output buffer
    while (ps2_read_byte(&dummy) != FALSE)
    {
        continue;
    }

    // Выключение приёма ввода
    ps2_write_byte_and_wait(portid, PS2_DISABLE_SCANNING);
    // Очистка Output buffer
    while (ps2_read_byte(&dummy) != FALSE)
    {
        continue;
    }

    int len = 0;
    uint8_t ident[2];
    ps2_write_byte_and_wait(portid, PS2_IDENTIFY);

    // Считаем identify
    for (int i = 0; i < 2; i ++)
    {
        out = 0;
        rc = ps2_read_byte(&out);
        if (rc == FALSE)
        {
            break;
        }

        ident[len++] = out;
    }

    // Standard PS/2 Mouse
	if (len == 1 && (ident[0] == 0x00))
	{
        *type = PS2_MOUSE;
		return 0;
	}

    if (len == 2 && ident[0] == 0xAB)
	{
		switch (ident[1])
		{
			case 0x41: // MF2 Keyboard (translated but my laptop uses this :))
			case 0x83: // MF2 Keyboard
			case 0xC1: // MF2 Keyboard
			case 0x84: // Thinkpad KB
                *type = PS2_KEYBOARD;
				return 0;
			default:
				break;
		}
	}

    return -2;
}

void ps2_device_setup(int portid, int type)
{
    switch (type)
    {
    case PS2_KEYBOARD:
        ps2_write_byte_and_wait(portid, PS2_KBD_SET_LEDS);
        ps2_write_byte_and_wait(portid, 0);

        ps2_write_byte_and_wait(portid, PS2_KBD_SET_SCAN_CODE_SET);
        ps2_write_byte_and_wait(portid, 2);
        break;
    
    default:
        break;
    }
}

int ps2_check_acpi()
{
    acpi_fadt_t* fadt = acpi_get_fadt();
    if (fadt)
    {
        uint64_t fadt_len = fadt->header.length;
        uint64_t iapc_flag_end = offsetof(acpi_fadt_t, boot_architecture_flags) + sizeof(fadt->boot_architecture_flags);

        //printk("Fadt len %i revision %i IAPC offset %i\n", fadt_len, fadt->header.revision, iapc_flag_end);

        // Если 
        // 1. Поле IA PC Boot Architecture Flags присутствует - Размер FADT больше либо равен его смещению
        // 2. Бит 1 в этом поле не установлен
        // Значит PS/2 контроллера нет  
        if ((fadt_len >= iapc_flag_end) && !(fadt->boot_architecture_flags & (1 << 1)))
        {
            // PS2 отсутствует
            return FALSE;
        }
    }

    return TRUE;
}

void init_ps2()
{
    int rc;
    int ports_availability[2] = {TRUE, FALSE};

    // Определим наличие PS/2 контроллера по ACPI
#if 0
    rc = ps2_check_acpi();
    if (rc == FALSE)
    {
        printk("No PS/2 Controller\n");
        return;
    }
#endif

    // Выключить оба порта
    ps2_cmd(PS2_DISABLE_PORT1);
    ps2_cmd(PS2_DISABLE_PORT2);

    // Очищаем Output Buffer
    uint8_t dummy;
    while (ps2_read_byte(&dummy) != FALSE)
    {
        continue;
    }

    // Set the Controller Configuration Byte
    uint8_t config;
	ps2_cmd(PS2_READ_CONFIG);
	ps2_read_byte(&config);
    config &= ~(PS2_IRQ_PORT1 | PS2_TRANSLATION_PORT1);
    ps2_cmd1(PS2_WRITE_CONFIG, config);

    // Perform Controller Self Test
    int test_result = ps2_perform_controller_test();
    if (test_result != TRUE)
    {
        printk("PS/2: Controller test failed\n");
        return;
    }
    // Говорят, что после теста может сброситься конфигурация
    ps2_cmd1(PS2_WRITE_CONFIG, config);

    // Есть ли у нас второй порт?
    int has_second_port = ps2_has_port2();
    printk("PS/2: Second port %i\n", has_second_port);
    if (has_second_port == TRUE)
    {
        ports_availability[1] = TRUE;
    }

    // Тестируем первый порт
    test_result = ps2_perform_interface_test(PS2_TEST_PORT1);
    if (test_result == FALSE)
    {
        // Помечаем порт 1 как недоступный
        printk("PS/2: First port test failed\n");
        ports_availability[0] = FALSE;
    }

    // Тестируем порт 2, если он есть
    if (has_second_port == TRUE)
    {
        test_result = ps2_perform_interface_test(PS2_TEST_PORT2);
        if (test_result == FALSE)
        {
            // Помечаем порт 2 как недоступный
            printk("PS/2: Second port test failed\n");
            ports_availability[1] = FALSE;
        }
    }

    if (ports_availability[0] == FALSE && ports_availability[1] == FALSE)
    {
        printk("PS/2: No ports avaliable\n");
        return;
    }

    // Считаем актуальную конфигурацию
    ps2_cmd(PS2_READ_CONFIG);
	ps2_read_byte(&config);

    int dev_type = 0;

    if (ports_availability[0] == TRUE)
    {
        ps2_cmd(PS2_ENABLE_PORT1);
        config |= PS2_IRQ_PORT1;
    }

    if (ports_availability[1] == TRUE)
    {
        ps2_cmd(PS2_ENABLE_PORT2);
        config |= PS2_IRQ_PORT2;
    }

    // Запишем конфигурацию
    ps2_cmd1(PS2_WRITE_CONFIG, config);

    // Identify + включение прерываний
    int irq_vectors[2] = {PS2_PORT1_IRQ_NUM, PS2_PORT2_IRQ_NUM};
    for (int i = 0; i < 2; i ++)
    {
        if (ports_availability[i] == TRUE)
        {
            rc = ps2_reset_and_identity(i, &dev_type);
            if (rc != 0)
            {
                printk("PS/2: Device %i reset result %i\n", i, -rc);
                continue;
            }

            switch (dev_type)
            {
                case PS2_KEYBOARD:
                    printk("PS/2: Found keyboard on port %i\n", i);
                    break;
                case PS2_MOUSE:
                    printk("PS/2: Found mouse on port %i\n", i);
                    break;
            }

            // Настроить устройство
            ps2_device_setup(i, dev_type);

            // Включение приема ввода
            ps2_write_byte_and_wait(i, PS2_ENABLE_SCANNING);
            // Очистка Output buffer
            while (ps2_read_byte(&dummy) != FALSE)
            {
                continue;
            }

            register_irq_handler(irq_vectors[i], ps2_device_irq_handler, dev_type);
        }
    }
}

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
    //[0x4E] = KRXK_L,  '-'
    [0x52] = KRXK_QUOTES,
    //[0x54] = KRXK_P,  [
    //[0x55] = KRXK_,   =
    [0x58] = KRXK_CAPS,
    [0x59] = KRXK_RSHIFT,
    [0x5A] = KRXK_ENTER,
    //[0x5B] = KRXK_ENTER,  ]
    [0x5D] = KRXK_BSLASH,
    [0x66] = KRXK_BKSP,


    [0x76] = KRXK_ESCAPE,

    [0x78] = KRXK_F11,

    [0x83] = KRXK_F7

};

void ps2_device_irq_handler(interrupt_frame_t* frame, void* data)
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
        uint8_t* keymap = ps2kbd_tag_E0 == 0 ? kbd_set2_mappings : NULL;

        uint8_t keycode_krx = keymap[keycode_ps2];
    
        // Добавить в очередь
        keyboard_add_event(keycode_krx, action);

        // Сбросить флаги
        ps2kbd_tag_E0 = 0;
        ps2kbd_tag_F0 = 0;
    }
}