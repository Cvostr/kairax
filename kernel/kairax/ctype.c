#include "ctype.h"

int isalpha(int character)
{
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}

int isdigit(int character)
{
    return (character >= '0' && character <= '9');
}

int isspace(int c)
{
	return c == ' ';
}