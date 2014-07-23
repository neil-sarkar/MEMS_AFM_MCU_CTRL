#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <ADuC7122.h>
#include "config.h"

// Bit Definitions
#define BIT0   0x01
#define BIT1   0x02
#define BIT2   0x04
#define BIT3   0x08
#define BIT4   0x10
#define BIT5   0x20
#define BIT6   0x40
#define BIT7   0x80
#define BIT8   0x100
#define BIT9   0x200
#define BIT10  0x400
#define BIT11  0x800
#define BIT12  0x1000
#define BIT13  0x2000
#define BIT14  0x4000
#define BIT15  0x8000
#define BIT16  0x10000
#define BIT17  0x20000
#define BIT18  0x40000
#define BIT19  0x80000
#define BIT20  0x100000
#define BIT21  0x200000
#define BIT22  0x400000
#define BIT23  0x800000
#define BIT24  0x1000000
#define BIT25  0x2000000
#define BIT26  0x4000000
#define BIT27  0x8000000
#define BIT28  0x10000000
#define BIT29  0x20000000
#define BIT30  0x40000000
#define BIT31  0x80000000

typedef enum
{
	false=0,
	true
} bool;

typedef unsigned char u8;
typedef char		  s8;

typedef unsigned int  u16;
typedef int 		  s16;

typedef unsigned short 	us16;
typedef short			ss16;

typedef unsigned int u32;
typedef int 		 s32;

/***** type definitions *****/
/*
Reference:
sizeof double
0008

sizeof float
0004

sizeof long
0004

sizeof int
0004

sizeof short
0002

sizeof char
0001
*/

#endif
