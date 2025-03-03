#include "kstdlib.h"
#include "types.h"
#include "ctype.h"
#include "string.h"

uint64_t align(uint64_t val, size_t alignment)
{
	if (val < alignment)
		return alignment;

	if (val % alignment != 0) {
        val += (alignment - (val % alignment));
    }

	return val;
}

uint64_t align_down(uint64_t val, size_t alignment)
{
	return val - (val % alignment);
}

void reverse(char *str){
  char tmp, *src, *dst;
  size_t len;

  if (str != NULL){
    len = strlen(str);

    if (len > 1){
      src = str;
      dst = src + len - 1;

      while(src < dst){
        tmp = *src;
        *src++ = *dst;
        *dst-- = tmp;
      }
    }
  }
}

static char destination[32] = {0};

char* itoa(int64 number, int base){

 	int count = 0;
  do {
      	int digit = number % base;
      	destination[count++] = (digit > 9) ? digit - 10 + 'A' : digit + '0';
    	} while ((number /= base) != 0);
    	destination[count] = '\0';
    	int i;
    	for (i = 0; i < count / 2; ++i) {
      	char symbol = destination[i];
      	destination[i] = destination[count - i - 1];
      	destination[count - i - 1] = symbol;
    	}
    	return destination;

}

int atoi(const char *s)
{
	return atol(s);
}

long atol(const char *s)
{
	return strtol(s, NULL, 10);
}

unsigned long int strtoul(const char *nptr, char **endptr, int base)
{
    unsigned long int val = 0;
    char *curpos = (char*) nptr;

    while (isspace(*curpos))
        curpos++;

    while (isdigit(*curpos)) {
        val = val * 10 + (*curpos++ - '0');
    }

    if (endptr != NULL) {
        *endptr = curpos;
    }

    return val;
}

long int strtol(const char *nptr, char **endptr, int base)
{
    char *curpos = (char*) nptr;
    int neg = 0;

    while (isspace(*curpos))
        curpos++;

    switch (*curpos) {
        case '-': 
            neg = 1;
        case '+':
            curpos++;
    }

    long int val = strtoul(curpos, endptr, base);
    
    return neg ? -val : val;
}

char* ulltoa(uint64_t number, int base)
{
  int count = 0;
  do {
      	long long digit = number % base;
      	destination[count++] = (digit > 9) ? digit + 'A' - 10 : digit + '0';
    	} while ((number /= base) != 0);
    	destination[count] = '\0';
    	int i;
    	for (i = 0; i < count / 2; ++i) {
      	char symbol = destination[i];
      	destination[i] = destination[count - i - 1];
      	destination[count - i - 1] = symbol;
    	}
    	return destination;
}