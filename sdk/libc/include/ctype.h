#ifndef _CTYPE_H_
#define _CTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

extern int isspace(int c);
extern int isdigit(int c);
extern int toupper(int c);
extern int isalpha(int c);
extern int isalnum(int c);

extern int isupper (int c);
extern int islower (int c);

#ifdef __cplusplus
}
#endif

#endif