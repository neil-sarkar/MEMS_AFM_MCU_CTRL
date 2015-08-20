#pragma once

#include "../global/global.h"

typedef enum
{
	STEP_FULL = 0,
	STEP_2_MICRO,
	STEP_4,
	STEP_8,
	STEP_16,
	STEP_32
} STEP_MODE;


void stpr_init(void);
void stpr_set_step(STEP_MODE stp_mode); // '0'
void stpr_step(void);										// '1'
void stpr_set_speed(u16 speed);					// '2' - followed by 16 bit value to go from 5KhZ to 20Hz
void stpr_cont(void);										// '3' - 
void stpr_sleep(void);									// '4'
void stpr_wake(void);										// '5'
void stpr_set_dir(u8 dir);							// '6' - f or b
void stpr_handler(void);
void stpr_exit_cont(void);							// '7'
