#ifndef __DAC_H
#define __DAC_H

#include "../global/global.h"

#define DAC_MAX_V		3.3f
#define DAC_MAX			4095

// DAC values are also used as array indicies, keep first DAC to 0
typedef enum
{
	dac0 = 0,
	dac1,
	dac2,
	dac3,
	dac4,
	dac5,
	dac6,
	dac7,
	dac8,
	dac9,
	dac10,
	dac11,
	NUM_DACS,
} dac;

typedef enum
{
	dac_Vref_AGND,
	dac_AVdd_AGND,
} dac_range;

typedef enum
{
	dac_enable,
	dac_disable,
} dac_state;

void dac_set_range (dac channel, dac_range range);

void dac_init (dac channel, dac_state state);

u8 dac_set_limit (dac channel, u16 new_limit);

u16 dac_get_limit (dac channel);

void dac_set_val (dac channel, u16 new_value);

u16 dac_get_val (dac channel);

#endif
