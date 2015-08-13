#include "stpr_DRV8834.h"

#define STP_CNT_100_US	100

struct stpr
{
	u32 speed;
	unsigned long delay;
	unsigned long delay_tmp;
};

static struct stpr s = {1, 	STP_CNT_100_US, STP_CNT_100_US};

extern volatile char G_exit_flag;
	
// the suffix _d is gpio direction bit
// the suffix _o is gpio output bit
#define REG_DIR				GP2DAT
#define BIT_DIR_d			BIT30		// 2.6
#define BIT_DIR_o  		(BIT_DIR_d >> 8)

#define REG_STP				GP2DAT
#define BIT_STP_d			BIT31		// 2.7
#define BIT_STP_o			(BIT_STP_d >> 8)

#define REG_SLEEP			GP3DAT
#define BIT_SLEEP_d		BIT28		// 3.4
#define BIT_SLEEP_o		(BIT_SLEEP_d >> 8)

#define REG_M0				GP3DAT
#define BIT_M0_d			BIT30		// 3.6
#define BIT_M0_o			(BIT_M0_d >> 8)

#define REG_M1				GP3DAT
#define BIT_M1_d			BIT25		// 3.1
#define BIT_M1_o			(BIT_M1_d >> 8)

#define SET_1(reg, bit) reg |= bit;
#define SET_0(reg, bit) reg &= ~bit;

void stpr_init()
{
	// set all gpios as outputs
	SET_1(REG_DIR, BIT_DIR_d);
	SET_1(REG_STP, BIT_STP_d);
	SET_1(REG_SLEEP, BIT_SLEEP_d);
	SET_1(REG_M0, BIT_M0_d);
	SET_1(REG_M1, BIT_M1_d);
	
	// initialize gpios
	SET_1(REG_DIR, BIT_DIR_o);
	// start out in sleep mode
	SET_0(REG_SLEEP, BIT_SLEEP_o);
	SET_0(REG_STP, BIT_STP_o);
	SET_0(REG_M1, BIT_M1_o);
	SET_0(REG_M0, BIT_M0_o);

}

void stpr_set_step(STEP_MODE stp_mode)
{
	// set to output
	SET_1(REG_M0, BIT_M0_d);
	
	switch(stp_mode)
	{
		case STEP_FULL:
			SET_0(REG_M1, BIT_M1_o);		//M1=0
			SET_0(REG_M0, BIT_M0_o);		//M0=0
			break;
		case STEP_2_MICRO:
			SET_0(REG_M1, BIT_M1_o);		//M1=0
			SET_1(REG_M0, BIT_M0_o);		//M0=1
			break;
		case STEP_4:
			SET_0(REG_M1, BIT_M1_o);		//M1=0
			SET_0(REG_M0, BIT_M0_d);		//M0=highZ
			break;
		case STEP_8:
			SET_1(REG_M1, BIT_M1_o);		//M1=1
			SET_0(REG_M0, BIT_M0_o);		//M0=0
			break;
		case STEP_16:
			SET_1(REG_M1, BIT_M1_o);		//M1=1
			SET_1(REG_M0, BIT_M0_o);		//M0=1
			break;
		case STEP_32:
			SET_1(REG_M1, BIT_M1_o);		//M1=1
			SET_0(REG_M0, BIT_M0_d);		//M0=highZ
			break;
		default:
			break;
	}
}

void stpr_set_dir(u8 dir)
{
	if (dir == 'f')
	{
		SET_1(REG_DIR, BIT_DIR_o);
	}
	else
	{
		SET_0(REG_DIR, BIT_DIR_o);
	}
}

void stpr_sleep()
{
	// active low sleep
	SET_0(REG_SLEEP, BIT_SLEEP_o);
}

void stpr_wake()
{
	// active low sleep
	SET_1(REG_SLEEP, BIT_SLEEP_o);
}

void stpr_cont()
{
	while (G_exit_flag == 0)
	{
		stpr_step();
	}
	
	G_exit_flag = 0;
}

void stpr_step()
{

	SET_1(REG_STP, BIT_STP_o);
	s.delay_tmp = s.delay;
	while(s.delay_tmp--) {};
	SET_0(REG_STP, BIT_STP_o);
	s.delay_tmp = s.delay;
	while(s.delay_tmp--) {};
}

void stpr_set_speed(u16 speed)
{
	s.speed = speed;
	s.delay = STP_CNT_100_US*s.speed;
}
