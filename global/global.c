/*
Global helper functions
*/

#include "global.h"

/************************************/
#define DELAY_1_MS		3900
#define DELAY_25_US		90		
			
void delay_ms(u16 delay)
{
	u32 wait = DELAY_1_MS*delay;
	while (wait--) {}	
}

void delay_25_us(u8 delay)
{
	u32 wait = DELAY_25_US*delay;
	while (wait--) {}	
}
/************************************/
