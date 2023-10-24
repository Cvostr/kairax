#include "stdio.h"
#include "time.h"

int main(int argc, char** argv) {

    time_t timesec = time(NULL);
    struct tm* t = gmtime(&timesec);

    printf("%02i:%02i:%02i   %i:%i:%i\n", t->tm_hour, 
									t->tm_min,
									t->tm_sec,
									t->tm_mday,
									t->tm_mon,
									t->tm_year + 1900);

    return 0;
}