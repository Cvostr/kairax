#include "stdio.h"
#include "stdarg.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "fcntl.h"
#include "limits.h"

static char destination[32] = {0};
static char temp[130];

void write_to_console(const char* buffer, int size) {
	temp[0] = 'w';
	temp[1] = size;

	for (int i = 0; i < size; i ++) {
		temp[2 + i] = buffer[i];
	}

	write(STDOUT_FILENO, temp, 2 + size);
}

int putchar(int ic) {

	
	return ic;
}

static int print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	int len = 0;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) != EOF)
			len ++;

	write_to_console(data, len);

	return 1;
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);
 
	int written = 0;
 
	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;
 
		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}
 
		const char* format_begun_at = format++;
 
		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
		} else if (*format == 'i') {
			format++;
			size_t integer = va_arg(parameters, size_t);
			char* str = itoa(integer, destination, 10);
			size_t len = strlen(str);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}
 
	va_end(parameters);
	return written;
}