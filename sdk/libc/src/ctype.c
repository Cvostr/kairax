#include "ctype.h"

int isspace(int c)
{
	return c == ' ';
}

int isdigit(int c)
{
	return ((unsigned int) c - '0') < 10;
}