/*
This a set of global functions that all drivers/applications
could use. An example is a simple busy-wait delay function.
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


/************************************/
void get_mcu_version (void)
{
	
}
/************************************/
