#include <unistd.h>
#include "stdio.h"
#include "getopt.h"

int main(int argc, char** argv)
{
    int rez = 0;

	int flag_a = 0;
	int flag_b = 0;
	int flag_d = 0;

	const struct option long_options[] = {
		{ "opta", no_argument, &flag_a, 1 },
		{ "optb", no_argument, &flag_b, 10 },
		{ "optc", optional_argument, NULL, 12 },
		{ "optd", required_argument, &flag_d, -121 },
		{ NULL, 0, NULL, 0}
	};

	char* shortopts = "ab:d";

	printf("getopt():\n");
	while ( (rez = getopt(argc, argv, shortopts)) != -1){
		switch (rez) {
		case 'a': printf("found argument \"a\".\n"); break;
		case 'b': printf("found argument \"b = %s\".\n", optarg); break;
		case 'd': printf("found argument \"d\"\n"); break;
		case '?': printf("Error found !\n"); break;
		}
	} 

	printf("getopt_long():\n");
	optind = 0;

	while ( (rez = getopt_long(argc, argv, shortopts,
		long_options, NULL)) != -1)
	{
		switch (rez) {
		case 'a': printf("found argument \"a\".\n"); break;
		case 'b': printf("found argument \"b = %s\".\n", optarg); break;
		case 'd': printf("found argument \"d\"\n"); break;
		case '?': printf("Error found !\n"); break;
		default:
			printf("output %i, optarg: %s\n", rez, optarg);
		}
	}

	printf("flag_a = %d\n", flag_a);
	printf("flag_b = %d\n", flag_b);
	printf("flag_d = %d\n", flag_d);
	printf("\n");
}