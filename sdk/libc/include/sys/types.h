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
typedef unsigned long 		dev_t;

typedef unsigned long int 	blkcnt_t;
typedef unsigned long int 	blksize_t;  

typedef signed long 		suseconds_t;
typedef signed long 		useconds_t;

typedef long int            time_t;
typedef long long  			pid_t;
typedef signed long         ssize_t;

typedef unsigned int        socklen_t;
typedef unsigned short      sa_family_t;

typedef long int            fpos_t;
typedef unsigned long int   clock_t;

#endif