#include "stpr_DRV8834.h"

#define MTR_DAT_REG		GP2DAT
#define MTR_DIR 		BIT22
#define MTR_PWR 		BIT23
#define MTR_DIR_DD 		(MTR_DIR<<8)
#define MTR_PWR_DD 		(MTR_PWR<<8)

void stpr_init()
{
	
}

void stpr_step(STEP_MODE stp_mode)
{
	switch(stp_mode)
	{
		case STEP_FULL:
			break;
		case STEP_2_MICRO:
			break;
		case STEP_4:
			break;
		case STEP_8:
			break;
		case STEP_16:
			break;
		case STEP_32:
			break;
		default:
			break;
	}
}
