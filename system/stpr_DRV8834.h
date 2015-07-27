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
void stpr_step(STEP_MODE stp_mode);
