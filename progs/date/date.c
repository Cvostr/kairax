#include "stdio.h"
#include "time.h"
#include "stdlib.h"
#include "string.h"
#include "sys/time.h"

char* months[] = {
	"jan",
	"feb",
	"mar",
	"apr",
	"may",
	"june",
	"july",
	"aug",
	"sept",
	"oct",
	"nov",
	"dec"
};

int get_month(char* month_str)
{
	if (strlen(month_str) != 3) {
		return -1;
	}

	for (int i = 0; i < 12; i ++) {
		if (strcmp(months[i], month_str) == 0) {
			return i;
		}
	}

	return -1;
}

int main(int argc, char** argv) {

	if (argc > 1) {

		if (strcmp(argv[1], "-set") == 0) {
			
			if (argc < 6) {
				printf("Not all supplied\n");
				return 1;
			} 

			int day = atoi(argv[2]);
			char* mon = argv[3];
			int year = atoi(argv[4]);
			char* time = argv[5];

			int month = get_month(mon);
			if (month == -1) {
				printf("Incorrect month value: %s\n", mon);
				return 2;
			}

			int hour, min, sec;
			sscanf(time, "%d;%d;%d", &hour, &min, &sec);

			printf("Setting date to %02i:%02i:%02i   %i %s %i\n", hour, min, sec, day, mon, year);

			struct tm ttim;
			ttim.tm_hour = hour;
			ttim.tm_min = min;
			ttim.tm_sec = sec;
			ttim.tm_year = year - 1900;
			ttim.tm_mday = day;
			ttim.tm_mon = month;

			time_t ttime = mktime(&ttim);
			
			struct timeval tv;
			tv.tv_sec = ttime;
			tv.tv_usec = 0;
			settimeofday(&tv, NULL);
		}

	} else {
		time_t timesec = time(NULL);
		struct tm* t = gmtime(&timesec);

		printf("%02i:%02i:%02i   %i %s %i\n", t->tm_hour, 
										t->tm_min,
										t->tm_sec,
										t->tm_mday,
										months[t->tm_mon],
										t->tm_year + 1900);
	}

    return 0;
}