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

// Terminal Special Characters (c_cc)
#define VINTR	0
#define VQUIT	1
#define VERASE	2
#define VKILL	3
#define VEOF	4
#define VTIME	5
#define VMIN	6
#define VSWTC	7
#define VSTART	8
#define VSTOP	9
#define VSUSP	10
#define VEOL	11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT	15
#define VEOL2	16

// c_lflag
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

// c_iflag
#define IGNBRK	0000001
#define BRKINT	0000002
#define IGNPAR	0000004
#define PARMRK	0000010
#define INPCK	0000020
#define ISTRIP	0000040
#define INLCR	0000100
#define IGNCR	0000200
#define ICRNL	0000400
#define IUCLC	0001000
#define IXON	0002000
#define IXANY	0004000
#define IXOFF	0010000
#define IMAXBEL	0020000
#define IUTF8	0040000

// c_oflag
#define OPOST		0000001
#define OLCUC       0000002
#define ONLCR	    0000004
#define OCRNL	    0000010
#define ONOCR	    0000020
#define ONLRET	    0000040
#define OFILL	    0000100
#define OFDEL	    0000200

#define TOSTOP	0000400
#define FLUSHO	0010000
#define IEXTEN	0100000

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
void cfmakeraw(struct termios *t) __THROW;

__END_DECLS

#endif