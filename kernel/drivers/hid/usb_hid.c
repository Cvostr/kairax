#include "hid.h"
#include "kstdlib.h"
#include "list/list.h"
#include "string.h"
#include "mem/kheap.h"

//#define HID_LOG_LOCAL_ITEMS
//#define HID_LOG_HEADER
//#define HID_LOG_IOF
//#define HID_LOG_ITEMS

int usb_hid_set_protocol(struct usb_device* device, struct usb_interface* interface, int protocol)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_CLASS;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = HID_SET_PROTOCOL;
	req.wValue = protocol;
	req.wIndex = interface->descriptor.bInterfaceNumber;
	req.wLength = 0;

    return usb_device_send_request(device, &req, NULL, 0);
}

int usb_hid_set_idle(struct usb_device* device, struct usb_interface* interface)
{
    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_CLASS;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_HOST_TO_DEVICE;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = HID_SET_IDLE;
	req.wValue = 0;
	req.wIndex = interface->descriptor.bInterfaceNumber;
	req.wLength = 0;

    return usb_device_send_request(device, &req, NULL, 0);
}

int usb_hid_get_report(struct usb_device* device, struct usb_interface* interface, uint8_t report_type, uint8_t report_id, void* report, uint32_t report_length)
{
    uint16_t value = (((uint16_t) report_type) << 8) | report_id;

    struct usb_device_request req;
	req.type = USB_DEVICE_REQ_TYPE_CLASS;
	req.transfer_direction = USB_DEVICE_REQ_DIRECTION_DEVICE_TO_HOST;
	req.recipient = USB_DEVICE_REQ_RECIPIENT_INTERFACE;
	req.bRequest = 1;
	req.wValue = value;
	req.wIndex = interface->descriptor.bInterfaceNumber;
	req.wLength = report_length;

    return usb_device_send_request(device, &req, report, report_length);
}

void hid_report_read(uint8_t *report, size_t pos, uint8_t bSize, hid_value_t *val)
{
    val->size = bSize;
    memcpy(&val->v.u32, &report[pos], bSize);
}

int32_t hid_get_signed_value(hid_value_t *val)
{
	switch (val->size)
	{
	case 1:
		return val->v.i8;
	case 2:
		return val->v.i16;
	case 3:
	case 4:
		return val->v.i32;
	}

	return 0;
}

uint16_t hid_report_read16u(uint8_t *report, size_t pos, uint8_t bSize)
{
    uint16_t res = 0;
    size_t sz = MIN(bSize, 2);
    memcpy(&res, &report[pos], sz);
    return res;
}

uint32_t hid_report_read32u(uint8_t *report, size_t pos, uint8_t bSize)
{
    uint32_t res = 0;
    size_t sz = MIN(bSize, 4);
    memcpy(&res, &report[pos], sz);
    return res;
}

int32_t hid_report_read32(uint8_t *report, size_t pos, uint8_t bSize)
{
    int32_t res = 0;
    size_t sz = MIN(bSize, 4);
    memcpy(&res, &report[pos], sz);
    return res;
}

struct hid_global_state {
    hid_value_t usage_page;
    hid_value_t logical_minimum;
    hid_value_t logical_maximum;
    hid_value_t physical_minimum;
    hid_value_t physical_maximum;

    hid_value_t unit;
    hid_value_t unit_exponent;

    hid_value_t report_size;
    hid_value_t report_id;
    hid_value_t report_count;
};

struct hid_local_state {
    list_t usages_stack;    
	hid_value_t usage_minimum;
	hid_value_t usage_maximum;
};

void hid_add_report_item(enum hid_report_item_type type, uint32_t val, list_t *items, struct hid_global_state *global_state, struct hid_local_state *local_state)
{
	struct hid_report_item *item;
	size_t usages = list_size(&local_state->usages_stack);

	// Это значение всегда представляем как signed
	int32_t logical_minimum = hid_get_signed_value(&global_state->logical_minimum);
	int64_t logical_maximum;
	// определяем знак
	if (logical_minimum < 0)
	{
		// Если минимальное значение меньше 0, то и максимальное тоже может быть меньше 0
		logical_maximum = hid_get_signed_value(&global_state->logical_maximum);
	}
	else
	{
		logical_maximum = global_state->logical_maximum.v.u32;
	}

	/*
	int32_t physical_minimum = hid_get_signed_value(&global_state->physical_minimum);
	int64_t physical_maximum = hid_get_signed_value(&global_state->physical_maximum);

	printk ("PHYSICAL (%i %i)\n", physical_minimum, physical_maximum);
	*/

	// Смотрим, есть ли usages
	if (usages > 0)
	{
		// Сохраняем сразу число объектов
		// Из него будем вычитать по 1 объекту за каждый объект
		// Так как в таком случае Report Count указывает на количество объектов
		uint32_t remaining_report_cnt = global_state->report_count.v.u32;
		// Проходимся по списку
		while ((usages = list_size(&local_state->usages_stack)) > 0)
		{
			uint32_t report_count = 1;

			// Если обрабатываем последний объект, то у него будут оставшиеся поля
			if (usages == 1)
				report_count = remaining_report_cnt;

			// Вычитаем кол-во полей
			remaining_report_cnt -= report_count;

			uint32_t usage = list_dequeue(&local_state->usages_stack);
			uint16_t usage_page = (usage >> 16);

			item = kmalloc(sizeof(struct hid_report_item));
			item->type = type;
			item->usage_page = usage_page > 0 ? usage_page : global_state->usage_page.v.u16;
			item->usage_id = usage & 0xFFFF;
			item->usage_minimum = local_state->usage_minimum.v.u32;
			item->usage_maximum = local_state->usage_maximum.v.u32;
			item->report_id = global_state->report_id.v.u8;
			item->report_size = global_state->report_size.v.u32;
			item->report_count = report_count;
			item->logical_minimum = logical_minimum;
			item->logical_maximum = logical_maximum;
			item->value = val;

			list_add(items, item);

#ifdef HID_LOG_ITEMS
			printk("USAGE PAGE %u USAGE ID %u REPORT ID %i REPORT_SZ %i COUNT %i LOGICAL (%i, %i) VAL %i\n", item->usage_page,
				item->usage_id,
				item->report_id,
				item->report_size,
				item->report_count, 
				item->logical_minimum,
				item->logical_maximum,
				val);
#endif
		}
	}
	else
	{
		// Работаем без usage_id
		item = kmalloc(sizeof(struct hid_report_item));
		item->type = type;
		item->usage_page = global_state->usage_page.v.u16;
		item->usage_id = 0;
		item->usage_minimum = local_state->usage_minimum.v.u32;
		item->usage_maximum = local_state->usage_maximum.v.u32;
		item->report_id = global_state->report_id.v.u8;
		item->report_size = global_state->report_size.v.u32;
		item->report_count = global_state->report_count.v.u32;
		item->logical_minimum = logical_minimum;
		item->logical_maximum = logical_maximum;
		item->value = val;

		list_add(items, item);

#ifdef HID_LOG_ITEMS
		printk("USAGE PAGE %u REPORT ID %i REPORT_SZ %i COUNT %i LOGICAL (%i, %i) VAL %i\n", 
			item->usage_page,
			item->report_id,
			item->report_size,
			item->report_count,
			item->logical_minimum,
			item->logical_maximum,
			val);
#endif
	}
}

void hid_parse_report(uint8_t* report, size_t length, list_t *items)
{
    struct hid_global_state global_state;
	memset(&global_state, 0, sizeof(struct hid_global_state));

    list_t global_state_stack;
	memset(&global_state_stack, 0, sizeof(list_t));

	struct hid_local_state local_state;
	memset(&local_state, 0, sizeof(struct hid_local_state));

    size_t pos = 0;
    while (pos < length)
    {
        uint8_t header = report[pos ++];

        uint8_t bSize   = header & 0b11;
        uint8_t bType   = (header >> 2) & 0b11;
        uint8_t bTag    = (header >> 4) & 0b1111;

#ifdef HID_LOG_HEADER
		printk("HID: bSize %i bType %i bTag %i\n", bSize, bType, bTag);
#endif

        if (bSize == 3)
            bSize = 4;

        if (bType == HID_REPORT_ITEM_TYPE_MAIN)
        {
			uint32_t val = hid_report_read32u(report, pos, bSize);
            switch (bTag)
            {
            case 0b1000:    
                // input
#ifdef HID_LOG_IOF
				printk("HID: INPUT\n");
#endif
				hid_add_report_item(INPUT, val, items, &global_state, &local_state);
                break;
            case 0b1001:  
#ifdef HID_LOG_IOF  
				printk("HID: OUTPUT\n");
#endif
				hid_add_report_item(OUTPUT, val, items, &global_state, &local_state);
                break;
            case 0b1011:    
                // feature
                break;
            case 0b1010:
                // collection
				//printk("HID: COLLECTION\n");
                break;
            case 0b1100:
                // end collection
				//printk("HID: END COLLECTION\n");
                break;
            default:
                break;
            }

			// Надо сбросить local state
			memset(&local_state, 0, sizeof(struct hid_local_state));
        }
        else if (bType == HID_REPORT_ITEM_TYPE_GLOBAL)
        {
            switch (bTag)
            {
            case 0b0000:
                hid_report_read(report, pos, bSize, &global_state.usage_page);
                break;
            case 0b0001:
                hid_report_read(report, pos, bSize, &global_state.logical_minimum);
                break;
            case 0b0010:
                hid_report_read(report, pos, bSize, &global_state.logical_maximum);
                break;
            case 0b0011:
                hid_report_read(report, pos, bSize, &global_state.physical_minimum);
                break;
            case 0b0100:
                hid_report_read(report, pos, bSize, &global_state.physical_maximum);
                break;
            case 0b0101:
                hid_report_read(report, pos, bSize, &global_state.unit_exponent);
                break;
            case 0b0110:
                hid_report_read(report, pos, bSize, &global_state.unit);
                break;
            case 0b0111:
                hid_report_read(report, pos, bSize, &global_state.report_size);
                break;
            case 0b1000:
                hid_report_read(report, pos, bSize, &global_state.report_id);
                break;
            case 0b1001:
                hid_report_read(report, pos, bSize, &global_state.report_count);
                break;
            case 0b1010:
                printk("PUSH\n");
                struct hid_global_state *copy = kmalloc(sizeof(struct hid_global_state));
                memcpy(copy, &global_state, sizeof(struct hid_global_state));
				list_add(&global_state_stack, copy);
                break;
            case 0b1011:
				struct hid_global_state *popd = list_pop(&global_state_stack);
				memcpy(&global_state, popd, sizeof(struct hid_global_state));
				kfree(popd);
                printk("POP\n");
                break;
            default:
                break;
            }
        }
        else if (bType == HID_REPORT_ITEM_TYPE_LOCAL)
        {
            switch (bTag)
            {
            case 0b0000:
                /* usage */
				uint32_t usage = hid_report_read32u(report, pos, bSize);
#ifdef HID_LOG_LOCAL_ITEMS
				printk("USAGE %i\n", usage);
#endif
				list_add(&local_state.usages_stack, usage);
                break;
            case 0b0001:
                // usage minimum
				hid_report_read(report, pos, bSize, &local_state.usage_minimum);
#ifdef HID_LOG_LOCAL_ITEMS
				printk("usage minimum %u\n", local_state.usage_minimum.v.u32);
#endif
                break;
            case 0b0010:
                // usage maximum
				hid_report_read(report, pos, bSize, &local_state.usage_maximum);
#ifdef HID_LOG_LOCAL_ITEMS
				printk("usage maximum %u\n", local_state.usage_maximum.v.u32);
#endif
                break;
            default:
                break;
            }
        }

        pos += (bSize);
    }
}

uint32_t hid_get_bits_unsigned(uint8_t *report, size_t offset, size_t size)
{
	uint32_t result = 0;

	for (size_t i = 0; i < size; i ++)
	{
		size_t byte_idx = offset / 8;
		size_t bit_idx = offset % 8;
		uint8_t byt = report[byte_idx];
		
		uint8_t bit = (byt >> bit_idx) & 1;
		result |= bit << i;
		offset ++;
	}

	return result;
}

int32_t hid_get_bits_signed(uint8_t *report, size_t offset, size_t size)
{
	uint32_t result = hid_get_bits_unsigned(report, offset, size);

	uint32_t sign_mask = 1 << (size - 1);
	if ((result & sign_mask) == sign_mask)
	{
		// Число отрицательное
		// Сначала получим маску для числа
		uint32_t digit_mask = (1 << size) - 1;
		// Число, состоящее из единиц
		uint32_t res = -1;
		res &= ~digit_mask;
		res |= result;
		return res;
	}

	return result;
}

void hid_process_report(void* private_data, uint8_t *report, size_t len, list_t *items, hid_data_handle_routine handler)
{
	size_t bits_offset = 0;

	struct list_node* current_node = items->head;
    struct hid_report_item* current = NULL;

    while (current_node != NULL) 
    {
        current = (struct hid_report_item*) current_node->element;

        for (uint32_t report_i = 0; report_i < current->report_count; report_i ++)
		{
			int64_t val;
			int is_unsigned = current->logical_minimum >= 0;
			uint16_t usage_base = current->usage_id > 0 ? current->usage_id : current->usage_minimum;

			if (is_unsigned)
				val = hid_get_bits_unsigned(report, bits_offset, current->report_size);
			else
				val = hid_get_bits_signed(report, bits_offset, current->report_size);
			
			// Вызов внешнего обработчика
			handler(private_data, current->usage_page, usage_base + report_i, val);

			bits_offset += current->report_size;
		}

        // Переход на следующий элемент
        current_node = current_node->next;
    }
}