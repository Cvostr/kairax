#ifndef _TTY_H
#define _TTY_H

#include "fs/vfs/file.h"
#include "fs/vfs/inode.h"

typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

#define CCSNUM 32

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
//
#define IEXTEN	0100000

#define OPOST		0000001
#define OLCUC       0000002
#define ONLCR	    0000004
#define OCRNL	    0000010
#define ONOCR	    0000020
#define ONLRET	    0000040
#define OFILL	    0000100
#define OFDEL	    0000200

// Terminal Special Characters
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

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

#define TCGETS      0x5401
#define TCSETS      0x5402 // TCSANOW
#define TCSETSW		0x5403 // TCSADRAIN
#define TCSETSF		0x5404 // TCSAFLUSH
#define TIOCSPGRP	0x5410

#define TIOCGWINSZ  0x540E
#define TIOCSWINSZ  0x540F

#define TIOCNOTTY	0x5422

#define ETX         3
#define FS          0x1C
#define BS         '\b'
#define NAK         0x15
#define SUB         032
#define ETB         0x17

struct pty;

void tty_init();
int master_file_close(struct inode *inode, struct file *file);
int slave_file_close(struct inode *inode, struct file *file);

int tty_create(struct file **master, struct file **slave);

ssize_t master_file_write(struct file* file, const char* buffer, size_t count, loff_t offset);
ssize_t master_file_read(struct file* file, char* buffer, size_t count, loff_t offset);

ssize_t slave_file_write(struct file* file, const char* buffer, size_t count, loff_t offset);
ssize_t slave_file_read(struct file* file, char* buffer, size_t count, loff_t offset);

int tty_ioctl(struct file* file, uint64_t request, uint64_t arg);

void tty_line_discipline_mw(struct pty* p_pty, const char* buffer, size_t count);
void tty_line_discipline_sw(struct pty* p_pty, const char* buffer, size_t count);

#endif