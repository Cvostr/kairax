#ifndef _BITS_TYPES_H
#define _BITS_TYPES_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		ushort;
typedef unsigned int		u_int;	
typedef unsigned long 		u_long;

typedef unsigned int        uid_t;
typedef unsigned int        gid_t;
typedef unsigned long int   ino_t;
typedef long int            off_t;
typedef unsigned int        mode_t;
typedef unsigned long int   nlink_t;

typedef long int            time_t;
typedef unsigned long long  pid_t;
typedef unsigned long 		ssize_t;

struct timespec
{
	time_t tv_sec;
  	long int tv_nsec;
};

#endif