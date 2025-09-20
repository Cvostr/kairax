#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "termios.h"
#include <sys/ioctl.h>
#include "string.h"
#include "fcntl.h"

struct baud_table {
	const char *str;
	speed_t val;
};

struct baud_table baud_rates[] = {
	{      "0", B0      },
	{     "50", B50     },
	{     "75", B75     },
	{    "110", B110    },
	{    "134", B134    },
	{  "134.5", B134    },
	{    "150", B150    },
	{    "200", B200    },
	{    "300", B300    },
	{    "600", B600    },
	{   "1200", B1200   },
	{   "1800", B1800   },
	{   "2400", B2400   },
	{   "4800", B4800   },
	{   "9600", B9600   },
	{  "19200", B19200  },
	{  "38400", B38400  },
	{  "57600", B57600  },
	{ "115200", B115200 },
	{ "230400", B230400 },
	{ "460800", B460800 },
	{ "921600", B921600 },
};

int TTY_FD;

const char* get_speed_str(struct termios *t) 
{
	speed_t ospeed = cfgetospeed(t);
	for (size_t j = 0; j < sizeof(baud_rates) / sizeof(*baud_rates); ++j) 
    {
		if (ospeed == baud_rates[j].val) 
        {
            return baud_rates[j].str;
		}
	}

    return NULL;
}

int set_speed(struct termios *t, const char *speed_str) 
{
	for (size_t j = 0; j < sizeof(baud_rates) / sizeof(*baud_rates); ++j) 
    {
		if (strcmp(speed_str, baud_rates[j].str) == 0) 
        {
			cfsetospeed(t, baud_rates[j].val);
			return 1;
		}
	}

	return 0;
}

void print_cc(struct termios *tm, char* charname, int chr)
{
    char ch = tm->c_cc[chr];

    char printable[10];

    if (ch == 0)
    {
        strcpy(printable, "<undef>");
    }
    else
    {
        printable[0] = '^';
        printable[1] = 'A' + ch - 1;
        printable[2] = '\0';
    }

    printf("%s = %s; ", charname, printable);
}

void print_flag(tcflag_t flag, char* optname, int opt)
{
    int sett = (flag & opt) == opt;
    if (sett == 1)
    {
        printf("%s ", optname);
    } 
    else
    {
        printf("-%s ", optname);
    }
}

void print_csize(tcflag_t flag)
{
    switch ((flag & CSIZE)) 
    {
		case CS5: printf("cs5 ");  break;
		case CS6: printf("cs6 ");  break;
		case CS7: printf("cs7 ");  break;
		case CS8: printf("cs8 "); break;
	}
}

int print_state()
{
    struct termios tmi;
    int rc = tcgetattr(TTY_FD, &tmi);
    if (rc != 0)
    {
        perror("Error getting tty settings");
        return rc;
    }

    const char* speed_str = get_speed_str(&tmi);
    printf("speed %s baud; ", speed_str);

    struct winsize wsize;
    ioctl(TTY_FD, TIOCGWINSZ, &wsize);
    printf("rows %d; columns %d;\n", wsize.ws_row, wsize.ws_col);

    print_cc(&tmi, "intr", VINTR);
    print_cc(&tmi, "quit", VQUIT);
    print_cc(&tmi, "erase", VERASE);
    print_cc(&tmi, "kill", VKILL);
    print_cc(&tmi, "eof", VEOF);
    print_cc(&tmi, "eol", VEOL);
    print_cc(&tmi, "eol2", VEOL2);
    print_cc(&tmi, "swtch", VSWTC);
    print_cc(&tmi, "start", VSTART);
    print_cc(&tmi, "stop", VSTOP);
    print_cc(&tmi, "susp", VSUSP);
    print_cc(&tmi, "rprnt", VREPRINT);
    print_cc(&tmi, "werase", VWERASE);
    print_cc(&tmi, "lnext", VLNEXT);
    print_cc(&tmi, "discard", VDISCARD);
    printf("\n");
    //print_cc(&tmi, "min", VLNEXT);
    //print_cc(&tmi, "min", VLNEXT);

    // cflags
    print_flag(tmi.c_cflag, "parenb", PARENB);
    print_flag(tmi.c_cflag, "parodd", PARODD);
    print_csize(tmi.c_cflag);
    print_flag(tmi.c_cflag, "cread", CREAD);
    printf("\n");

    print_flag(tmi.c_iflag, "ignbrk", IGNBRK);
    print_flag(tmi.c_iflag, "brkint", BRKINT);
    print_flag(tmi.c_iflag, "ignpar", IGNPAR);
    print_flag(tmi.c_iflag, "parmrk", PARMRK);
    print_flag(tmi.c_iflag, "inpck", INPCK);
    print_flag(tmi.c_iflag, "istrip", ISTRIP);
    print_flag(tmi.c_iflag, "inlcr", INLCR);
    print_flag(tmi.c_iflag, "igncr", IGNCR);
    print_flag(tmi.c_iflag, "icrnl", ICRNL);
    print_flag(tmi.c_iflag, "ixon", IXON);
    print_flag(tmi.c_iflag, "ixoff", IXOFF);
    print_flag(tmi.c_iflag, "iuclc", IUCLC);
    print_flag(tmi.c_iflag, "ixany", IXANY);
    print_flag(tmi.c_iflag, "imaxbel", IMAXBEL);
    print_flag(tmi.c_iflag, "iutf8", IUTF8);
    printf("\n");

    print_flag(tmi.c_oflag, "opost", OPOST);
    print_flag(tmi.c_oflag, "olcuc", OLCUC);
    print_flag(tmi.c_oflag, "ocrnl", OCRNL);
    print_flag(tmi.c_oflag, "onlcr", ONLCR);
    print_flag(tmi.c_oflag, "onocr", ONOCR);
    print_flag(tmi.c_oflag, "onlret", ONLRET);
    print_flag(tmi.c_oflag, "ofill", OFILL);
    print_flag(tmi.c_oflag, "ofdel", OFDEL);
    printf("\n");

    print_flag(tmi.c_lflag, "isig", ISIG);
    print_flag(tmi.c_lflag, "icanon", ICANON);
    print_flag(tmi.c_lflag, "iexten", IEXTEN);
    print_flag(tmi.c_lflag, "echo", ECHO);
    print_flag(tmi.c_lflag, "echoe", ECHOE);
    print_flag(tmi.c_lflag, "echok", ECHOK);
    print_flag(tmi.c_lflag, "echonl", ECHONL);
    print_flag(tmi.c_lflag, "noflsh", NOFLSH);
    print_flag(tmi.c_lflag, "xcase", XCASE);
    //print_flag(tmi.c_lflag, "tostop", TOSTOP);
    print_flag(tmi.c_lflag, "echoprt", ECHOPRT);
    print_flag(tmi.c_lflag, "echoctl", ECHOCTL);
    print_flag(tmi.c_lflag, "echoke", ECHOKE);
    //print_flag(tmi.c_lflag, "flusho", FLUSHO);
    //print_flag(tmi.c_lflag, "extproc", EXTPROC);
    printf("\n");

    return 0;
}

int handle_flag(tcflag_t *flag, char *flagname, char *arg, int flagbit)
{
    int minus = 0;
    int equals = 0;

    if (arg[0] == '-')
    {
        minus = 1;
        arg += 1;
    }
    
    equals = strcmp(arg, flagname) == 0;

    if (equals)
    {
        if (minus)
        {
            *flag &= ~flagbit;
        }
        else
        {
            *flag |= flagbit;
        }
    }
}

void set_csize(tcflag_t *flag, const char *flagname, char *arg, int flagbit)
{
    if (strcmp(arg, flagname) == 0)
    {
        *flag &= ~(CSIZE);
        *flag |= flagbit;
    }
}

int main(int argc, char** argv) 
{
    struct termios tmi;
    int rc;
    int fd;
    int arg_i = 1;

    // По умолчанию работаем с stdout
    TTY_FD = STDOUT_FILENO;

    if (argc >= (arg_i + 2) && (strcmp(argv[arg_i], "-F") == 0))
    {
        fd = open(argv[arg_i + 1], O_RDONLY, 0);
        if (fd == -1)
        {
            perror("stty: Error opening file");
            return 1;
        }

        if (isatty(fd) == 0)
        {
            printf("stty: File is not a tty\n");
            return 1;
        }

        TTY_FD = fd;

        arg_i += 2;
    }

    if (argc == arg_i + 1 && strcmp(argv[arg_i], "-a") == 0)
    {
        rc = print_state();
        if (rc != 0)
        {
            return 1;
        }

        return 0;
    }

    rc = tcgetattr(TTY_FD, &tmi);
    if (rc != 0)
    {
        perror("stty: Error getting tty settings");
        return rc;
    }

    for (; arg_i < argc; arg_i ++)
    {
        char* arg = argv[arg_i];

        if (strcmp(arg, "cooked") == 0)
        {
            tmi.c_iflag |= ICRNL | BRKINT;
			tmi.c_oflag |= OPOST;
			tmi.c_lflag |= ISIG | ICANON;
            continue;
        }

        if (strcmp(arg, "raw") == 0)
        {
            tmi.c_iflag = 0;
            tmi.c_oflag &= ~OPOST;
            tmi.c_lflag &= ~(ICANON | ISIG | IUCLC | XCASE);
            continue;
        }

        if (strcmp(arg, "size") == 0)
        {
            struct winsize wsize;
            ioctl(TTY_FD, TIOCGWINSZ, &wsize);
            printf("%d %d\n", wsize.ws_row, wsize.ws_col);
            continue;
        }

        if (arg[0] >= '0' && arg[0] < '9') 
        {
			if (set_speed(&tmi, arg)) {
				arg_i++;
				continue;
			}
            else
            {
                printf("stty: nvalid argument '%s'\n", arg);
                return 1;
            }
		}

        handle_flag(&tmi.c_cflag, "parenb", arg, PARENB);
        handle_flag(&tmi.c_cflag, "parodd", arg, PARODD);
        handle_flag(&tmi.c_cflag, "cread", arg, CREAD);
        set_csize(&tmi.c_cflag, "cs5", arg, CS5);
        set_csize(&tmi.c_cflag, "cs6", arg, CS6);
        set_csize(&tmi.c_cflag, "cs7", arg, CS7);
        set_csize(&tmi.c_cflag, "cs8", arg, CS8);

        handle_flag(&tmi.c_iflag, "istrip", arg, ISTRIP);
        handle_flag(&tmi.c_iflag, "inlcr", arg, INLCR);
        handle_flag(&tmi.c_iflag, "igncr", arg, IGNCR);
        handle_flag(&tmi.c_iflag, "icrnl", arg, ICRNL);
        handle_flag(&tmi.c_iflag, "iuclc", arg, IUCLC);


        handle_flag(&tmi.c_oflag, "olcuc", arg, OLCUC);
        handle_flag(&tmi.c_oflag, "opost", arg, OPOST);
        handle_flag(&tmi.c_oflag, "onlcr", arg, ONLCR);
        handle_flag(&tmi.c_oflag, "ocrnl", arg, OCRNL);
        handle_flag(&tmi.c_oflag, "onocr", arg, ONOCR);
        handle_flag(&tmi.c_oflag, "onlret", arg, ONLRET);
        handle_flag(&tmi.c_oflag, "ofill", arg, OFILL);
        handle_flag(&tmi.c_oflag, "ofdel", arg, OFDEL);


        handle_flag(&tmi.c_lflag, "isig", arg, ISIG);
        handle_flag(&tmi.c_lflag, "icanon", arg, ICANON);
        handle_flag(&tmi.c_lflag, "iexten", arg, IEXTEN);
        handle_flag(&tmi.c_lflag, "echo", arg, ECHO);
        handle_flag(&tmi.c_lflag, "echoe", arg, ECHOE);
        handle_flag(&tmi.c_lflag, "echok", arg, ECHOK);
        handle_flag(&tmi.c_lflag, "echonl", arg, ECHONL);
    }

    tcsetattr(TTY_FD, TCSADRAIN, &tmi);

    return 0;
}