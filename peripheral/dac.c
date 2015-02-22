#include "dac.h"

static u16 dac_limit [NUM_DACS] = {0};

void dac_set_range (dac channel, dac_range range)
{

	u16 reg_val = 0x00;
	
	switch (range)
	{
		case dac_Vref_AGND:
			reg_val &= ~(BIT0 | BIT1);
			
			break;
		
		case dac_AVdd_AGND:
			reg_val |= BIT0 | BIT1;
			
			break;
			
		default:
			break;
	
	}
	
	switch (channel)
	{
	
		case dac0:
			/* set range */
			DAC0CON = reg_val;
			
			break;

		case dac1:
			/* set range */
			DAC1CON = reg_val;
			
			break;

		case dac2:
			/* set range */
			DAC2CON = reg_val;
		
			break;

		case dac3:
			/* set range */
			DAC3CON = reg_val;
		
			break;

		case dac4:
			/* set range */
			DAC4CON = reg_val;
		
			break;

		case dac5:
			/* set range */
			DAC5CON = reg_val;
		
			break;

		case dac6:
			/* set range */
			DAC6CON = reg_val;
		
			break;

		case dac7:
			/* set range */
			DAC7CON = reg_val;
		
			break;

		case dac8:
			/* set range */
			DAC8CON = reg_val;
		
			break;

		case dac9:
			/* set range */
			DAC9CON = reg_val;
		
			break;

		case dac10:
			/* set range */
			DAC10CON = reg_val;
		
			break;

		case dac11:
			/* set range */
			DAC11CON = reg_val;
		
			break;

		default:
			break;

	}

}

void dac_init (dac channel, dac_state state)
{

	u16 reg_val = 0x00;
	
	switch (state)
	{
	
		case dac_enable:
			reg_val |= BIT4;
			
			break;
			
		case dac_disable:
			reg_val &= ~BIT4;
			
			break;
			
		default:
			break;
	
	}
	
	switch (channel)
	{
	
		/* enable/disable */
		case dac0:
			DAC0CON |= reg_val;
			
			break;

		case dac1:
			DAC1CON |= reg_val;
			
			break;

		case dac2:
			DAC2CON |= reg_val;
		
			break;

		case dac3:
			DAC3CON |= reg_val;
		
			break;

		case dac4:
			DAC4CON |= reg_val;
		
			break;

		case dac5:
			DAC5CON |= reg_val;
		
			break;

		case dac6:
			DAC6CON |= reg_val;
		
			break;

		case dac7:
			DAC7CON |= reg_val;
		
			break;

		case dac8:
			DAC8CON |= reg_val;
		
			break;

		case dac9:
			DAC9CON |= reg_val;
		
			break;

		case dac10:
			DAC10CON |= reg_val;
		
			break;

		case dac11:
			DAC11CON |= reg_val;
		
			break;

		default:
			break;

	}	
}

u8 dac_set_limit (dac channel, u16 new_limit)
{
	// Uses enum as array index to set DAC limit
	if (new_limit <= DAC_MAX){
		dac_limit [channel] = new_limit;		
		return 0;
	}
	return 1;
}

u16 dac_get_limit (dac channel)
{
	return dac_limit [channel];
}

void dac_set_val (dac channel, u16 new_value)
{
	// Check to make sure we are not passing DAC limits
	if (channel != DAC_ZOFFSET_COARSE && channel != DAC_X1 && channel != DAC_Y1)
	{
		if (new_value > dac_limit[channel]){
			return;
		}
	}
	
	switch (channel)
	{

		case dac0:
			/* set value */
			DAC0DAT  = (new_value << 16);
			
			break;

		case dac1:
			/* set value */
			DAC1DAT  = (new_value << 16);
			
			break;

		case dac2:
			/* set value */
			DAC2DAT  = (new_value << 16);
		
			break;

		case dac3:
			/* set value */
			DAC3DAT  = (new_value << 16);
		
			break;

		case dac4:
			/* set value */
			DAC4DAT  = (new_value << 16);
		
			break;

		case dac5:
			/* set value */
			DAC5DAT  = (new_value << 16);
		
			break;

		case dac6:
			/* set value */
			DAC6DAT  = (new_value << 16);
		
			break;

		case dac7:
			/* set value */
			DAC7DAT  = (new_value << 16);
		
			break;

		case dac8:
			/* set value */
			DAC8DAT  = (new_value << 16);
		
			break;

		case dac9:
			/* set value */
			DAC9DAT  = (new_value << 16);
		
			break;

		case dac10:
			/* set value */
			DAC10DAT  = (new_value << 16);
		
			break;

		case dac11:
			/* set value */
			DAC11DAT  = (new_value << 16);
		
			break;

		default:
			break;

	}

}

u16 dac_get_val (dac channel)
{
	u16 dac_val;

	switch (channel)
	{

		case dac0:
			/* get value */
			dac_val = (DAC0DAT >> 16) & 0xFFF;
			
			break;

		case dac1:
			/* get value */
			dac_val = (DAC1DAT >> 16) & 0xFFF;
			
			break;

		case dac2:
			/* get value */
			dac_val = (DAC2DAT >> 16) & 0xFFF;
		
			break;

		case dac3:
			/* get value */
			dac_val = (DAC3DAT >> 16) & 0xFFF;
		
			break;

		case dac4:
			/* get value */
			dac_val = (DAC4DAT >> 16) & 0xFFF;
		
			break;

		case dac5:
			/* get value */
			dac_val = (DAC5DAT >> 16) & 0xFFF;
		
			break;

		case dac6:
			/* get value */
			dac_val = (DAC6DAT >> 16) & 0xFFF;
		
			break;

		case dac7:
			/* get value */
			dac_val = (DAC7DAT >> 16) & 0xFFF;
		
			break;

		case dac8:
			/* get value */
			dac_val = (DAC8DAT >> 16) & 0xFFF;
		
			break;

		case dac9:
			/* get value */
			dac_val = (DAC9DAT >> 16) & 0xFFF;
		
			break;

		case dac10:
			/* get value */
			dac_val = (DAC10DAT >> 16) & 0xFFF;
		
			break;

		case dac11:
			/* get value */
			dac_val = (DAC11DAT >> 16) & 0xFFF;
		
			break;

		default:
			break;

	}

	return dac_val;
}
