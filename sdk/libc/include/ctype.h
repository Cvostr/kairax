#ifndef _CTYPE_H_
#define _CTYPE_H

#include <sys/cdefs.h>

__BEGIN_DECLS

extern int isspace(int c);
extern int isdigit(int c);
extern int toupper(int c);
extern int tolower(int c);
extern int isalpha(int c);
extern int isalnum(int c);

extern int isupper (int c);
extern int islower (int c);

extern int isascii (int c);
extern int toascii (int c);

__END_DECLS

#endif