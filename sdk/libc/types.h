#ifndef _BITS_TYPES_H
#define _BITS_TYPES_H

typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef signed short int    int16_t;
typedef unsigned short int  uint16_t;
typedef signed int          int32_t;
typedef unsigned int        uint32_t;
typedef signed long int     int64_t;
typedef unsigned long int   uint64_t;

typedef unsigned int        uid_t;
typedef unsigned int        gid_t;
typedef unsigned long int   ino_t;
typedef long int            off_t;
typedef unsigned int        mode_t;
typedef unsigned long int   nlink_t;

typedef long int            time_t;
typedef unsigned long long  id_t;
typedef unsigned long 		ssize_t;

struct timespec
{
	time_t tv_sec;
  	long int tv_nsec;
};

#endif