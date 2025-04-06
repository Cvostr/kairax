#ifndef _TERMIOS_H
#define _TERMIOS_H

#include <sys/cdefs.h>

__BEGIN_DECLS

typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

#define CCSNUM 32

#define ISIG	0000001
#define ICANON	0000002
#define XCASE	0000004
#define ECHO	0000010
#define ECHOE	0000020
#define ECHOK	0000040
#define ECHONL	0000100
#define NOFLSH	0000200
#define ECHOCTL	0001000
#define ECHOPRT	0002000
#define ECHOKE	0004000

struct termios {
    
    tcflag_t c_iflag;		/* input mode flags */
    tcflag_t c_oflag;		/* output mode flags */
    tcflag_t c_cflag;		/* control mode flags */
    tcflag_t c_lflag;		/* local mode flags */
    cc_t c_line;			/* line discipline */
    cc_t c_cc[CCSNUM];		/* control characters */
    speed_t c_ispeed;		/* input speed */
    speed_t c_ospeed;		/* output speed */
};

#define TCSANOW		0
#define TCSADRAIN	1
#define TCSAFLUSH	2

int tcgetattr(int fd, struct termios *termios_p) __THROW;
int tcsetattr(int fd, int optional_actions, struct termios *termios_p) __THROW;

__END_DECLS

#endif