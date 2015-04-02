#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <ADuC7122.h>
#include "config.h"
#include "sys_config.h"

// Bit Definitions
#define BIT0   0x01u
#define BIT1   0x02u
#define BIT2   0x04u
#define BIT3   0x08u
#define BIT4   0x10u
#define BIT5   0x20u
#define BIT6   0x40u
#define BIT7   0x80u
#define BIT8   0x100u
#define BIT9   0x200u
#define BIT10  0x400u
#define BIT11  0x800u
#define BIT12  0x1000u
#define BIT13  0x2000u
#define BIT14  0x4000u
#define BIT15  0x8000u
#define BIT16  0x10000u
#define BIT17  0x20000u
#define BIT18  0x40000u
#define BIT19  0x80000u
#define BIT20  0x100000u
#define BIT21  0x200000u
#define BIT22  0x400000u
#define BIT23  0x800000u
#define BIT24  0x1000000u
#define BIT25  0x2000000u
#define BIT26  0x4000000u
#define BIT27  0x8000000u
#define BIT28  0x10000000u
#define BIT29  0x20000000u
#define BIT30  0x40000000u
#define BIT31  0x80000000u

typedef enum
{
	false=0,
	true
} bool;

typedef unsigned char 	u8;
typedef signed char		s8;

typedef unsigned short 	u16;
typedef short 		  	s16;

typedef unsigned int 	u32;
typedef int 		 	s32;

void delay_25_us (u8 delay);
void delay_1_ms (u16 delay);

#define CNT_1_MS		3900
#define CNT_25_US		90		
			
__inline void DELAY_MS(u16 delayCnt)
{
		u32 delay = CNT_1_MS*delayCnt;
		while (delay--) {}
}

__inline void DELAY_25_US(u16 delayCnt) 	
{
	u32 delay = CNT_25_US*delayCnt;
	while (delay--) {}
}

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
