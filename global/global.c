/*
This a set of global functions that all drivers/applications
could use. An example is a simple busy-wait delay function.
*/

#include "global.h"

/************************************/
#define DELAY_1_MS	3900
void delay_ms(u16 delay)
{
	u32 ms_wait = DELAY_1_MS*delay;
	while (ms_wait--) {}	
}
/************************************/

/************************************/
void get_mcu_version (void)
{
	
}
/************************************/
