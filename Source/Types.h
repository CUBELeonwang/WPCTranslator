/*subfile:  types.h  *********************************************************/
/*  special types  By George*/

#ifndef STD_TYPES
# define STD_TYPES
typedef char I1;           /* 1 byte signed integer */
typedef short I2;          /* 2 byte signed integer */
typedef long I4;           /* 4 byte signed integer */
typedef int IX;            /* default signed integer */
typedef unsigned char U1;  /* 1 byte unsigned integer */
typedef unsigned short U2; /* 2 byte unsigned integer */
typedef unsigned long U4;  /* 4 byte unsigned integer */
typedef unsigned UX;       /* default unsigned integer */
typedef float R4;          /* 4 byte real value */
typedef double R8;         /* 8 byte real value */
typedef long double RX;    /* 10 byte real value (extended precision) */
#endif

#define LINELEN 144
#define NAMELEN 16
