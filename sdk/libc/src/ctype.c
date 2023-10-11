#include "ctype.h"

int isspace(int c)
{
	return c == ' ';
}

int isdigit(int c)
{
	return ((unsigned int) c - '0') < 10;
}

int isalpha(int c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ? 1 : 0);
}

int isalnum(int c)
{
	return isdigit(c) || isalpha(c);
}

int toupper(int c) 
{
	if ( (unsigned int) (c - 'a') < 26u )
    	c += 'A' - 'a';
  	return c;
}

int isupper (int c)
{
	unsigned char x = c & 0xff;
  	return (x >= 'A' && x <= 'Z');
}

int islower (int c)
{
  	unsigned char x = c & 0xff;
  	return (x >= 'a' && x <= 'z');
}