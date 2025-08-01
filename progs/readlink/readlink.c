#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "string.h"

#define PATHBUFFER_SIZE 4096
char pathbuffer[PATHBUFFER_SIZE];

int main(int argc, char** argv) 
{
    if (argc < 2)
    {
        printf("readlink: missing operand\n");
        return 1;
    }

    char* name = argv[1];

    if (readlink(name, pathbuffer, PATHBUFFER_SIZE) < 0) 
    {
		perror("readlink");
		return 1;
	}

	printf("%s\n", pathbuffer);
	return 0;
}