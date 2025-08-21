#include "ps2.h"
#include "interrupts/pic.h"
#include "io.h"
#include "mem/kheap.h"
#include "sync/spinlock.h"
#include "string.h"
#include "dev/interrupts.h"
#include "interrupts/ioapic.h"
#include "dev/acpi/acpi.h"
#include "kairax/stdio.h"

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
        ps2_kbd_setup(portid);
        break;
    case PS2_MOUSE:
        ps2_mouse_setup(portid);
        break;
    default:
        break;
    }
}

int ps2_check_acpi()
{
    acpi_fadt_t* fadt = acpi_get_fadt();

    // Если есть таблица FADT - выполняем проверку
    // Иначе считаем, что контроллер i8042 есть по умолчанию
    if (fadt)
    {
        uint64_t fadt_len = fadt->header.length;
        uint64_t iapc_flag_end = offsetof(acpi_fadt_t, boot_architecture_flags) + sizeof(fadt->boot_architecture_flags);

        printk("Fadt len %i revision %i IAPC offset %i\n", fadt_len, fadt->header.revision, iapc_flag_end);

        if (fadt->header.revision < 2)
        {
            // Если ACPI версии ниже 2, то считаем, что i8042 есть по умолчанию
            return TRUE;
        }

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
#if 1
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

            // Настроить устройство
            ps2_device_setup(i, dev_type);

            // Включение приема ввода
            ps2_write_byte_and_wait(i, PS2_ENABLE_SCANNING);
            // Очистка Output buffer
            while (ps2_read_byte(&dummy) != FALSE)
            {
                continue;
            }

            // Выбор обработчика прерывания в зависимости от типа устройства
            void* irq_handler = NULL;
            switch (dev_type)
            {
            case PS2_KEYBOARD:
                irq_handler = ps2_kbd_irq_handler;
                break;
            case PS2_MOUSE:
                irq_handler = ps2_mouse_irq_handler;
                break;
            default:
                break;
            }

            // Установить обработчик прерывания
            register_irq_handler(irq_vectors[i], irq_handler, dev_type);
        }
    }
}