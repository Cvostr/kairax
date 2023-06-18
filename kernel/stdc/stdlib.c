#include "stdlib.h"
#include "types.h"
#include "string.h"

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