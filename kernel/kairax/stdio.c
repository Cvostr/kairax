#include "stdio.h"
#include "string.h"
#include "kstdlib.h"
#include "drivers/video/video.h"
#include "proc/syscalls.h"
#include "misc/kterm/vgaterm.h"

extern struct vgaconsole* current_console;

int getch()
{
	char ch;
	sys_read_file(0, &ch, 1);
	return ch;
}

char* kfgets(char* buffer, size_t len)
{
	char c;
	size_t i = 0;
	do {
		sys_read_file(0, &c, 1);
		buffer[i++] = c;
	} while (c != '\n' && i < len);

	buffer[i - 1] = '\0';

	return buffer;
}

static int print_console_func(const char* data, size_t length, void* arg) 
{
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		console_print_char(current_console, bytes[i], 170, 170, 170);
	return 1;
}

static int print_stdout_func(const char* data, size_t length, void* arg) 
{
	sys_write_file(1, data, length);
	return 1;
}

static int sprint_func(const char* data, size_t length, void* arg) 
{
	strncat((char*) arg, data, length);
	return 1;
}

int printk(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);
 
	int written = printf_generic(print_console_func, INT_MAX, NULL, format, parameters);
 
	va_end(parameters);
	return written;
}

int printf_stdout(const char* format, ...)
{
	va_list parameters;
	va_start(parameters, format);
 
	int written = printf_generic(print_stdout_func, INT_MAX, NULL, format, parameters);
 
	va_end(parameters);
	return written;
}

int sprintf(char* buffer, size_t buffersz, const char* format, ...)
{
	va_list parameters;
	va_start(parameters, format);

	buffer[0] = 0;
	int written = printf_generic(sprint_func, buffersz, buffer, format, parameters);
 
	va_end(parameters);
	return written;
}

int printf_generic(int (*f) (const char* str, size_t len, void* arg), size_t max, void* arg, const char* format, va_list args)
{
	int written = 0;
	char tempbuff[32];
 
	while (*format != '\0') 
	{
		// оставшееся место
		int64_t maxrem = max - written;
 
		if (format[0] != '%' || format[1] == '%') 
		{
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				return -1;
			}
			if (!f(format, amount, arg))
				return -1;
			format += amount;
			written += amount;
			continue;
		}
 
		const char* format_begun_at = format++;
 
		if (*format == 'c') {
			format++;
			char c = (char) va_arg(args, int);
			if (!maxrem) {
				return -1;
			}
			if (!f(&c, sizeof(c), arg))
				return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(args, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				return -1;
			}
			if (!f(str, len, arg))
				return -1;
			written += len;
		} else if (*format == 'i') {
			format++;
			int integer = va_arg(args, int);
			char* str = lltoa(integer, tempbuff, 10);
			size_t len = strlen(str);
			if (maxrem < len) {
				return -1;	
			}
			if (!f(str, len, arg))
				return -1;
			written += len;
		} else if (*format == 'x') {
			format++;
			int64 integer = va_arg(args, unsigned long);
			char* str = ulltoa(integer, tempbuff, 16);
			size_t len = strlen(str);
			if (maxrem < len) {
				return -1;
			}
			if (!f(str, len, arg))
				return -1;
			written += len;
		} else if (*format == 'u') {
			format++;
			size_t integer = va_arg(args, size_t);
			char* str = ulltoa(integer, tempbuff, 10);
			size_t len = strlen(str);
			if (maxrem < len) {
				return -1;
			}
			if (!f(str, len, arg))
				return -1;
			written += len;
		
		} else if (*format == 'p') {
			format++;
			size_t integer = va_arg(args, uintptr_t);
			char* str = ulltoa(integer, tempbuff, 16);
			size_t len = strlen(str);
			if (maxrem < len) {
				return -1;
			}
			if (!f(str, len, arg))
				return -1;
			written += len;
		
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				return -1;
			}
			if (!f(format, len, arg))
				return -1;
			written += len;
			format += len;
		}
	}

	return written;
}