#include "adc.h"

void adc_init() 
{
	u16 delay = 2000;
	ADCCON = BIT5;
	// wait for ADC to be fully powered on
	while (delay--);

	// continuous software conversion
	//ADCCON = BIT0 | BIT1 | BIT5 | BIT9 | BIT11;
	// 2.5V ref
   	REFCON |= BIT1;
}

void adc_start_conv(adc channel)
{
   	// single software conversion
	// ADCCON |= BIT0 | BIT1 | BIT5 | BIT7 | BIT9 | BIT11;

	u16 reg_val = 0x00;
	// continuous conversion
	reg_val |= BIT2 | BIT5 | BIT7 | BIT9 | BIT11;

	switch (channel) {
		case adc0:
			ADCCP = BIT1;
			break;

		case adc1:
			ADCCP = BIT0 | BIT1;
			break;

		case adc2:
			ADCCP = BIT2;
			break;

		case adc3:
			ADCCP = BIT0 | BIT2;
			break;

		case adc4:
			ADCCP = BIT1 | BIT2;
			break;

		case adc5:
			ADCCP = BIT0 | BIT1 | BIT2;
			break;

		case adc6:
			ADCCP = BIT3;
			break;

		case adc7:
			ADCCP = BIT0 | BIT3;
			break;

		case adc8:
			ADCCP = BIT1 | BIT3;
			break;

		case adc9:
			ADCCP = BIT0 | BIT1 | BIT3;
			break;

		case padc0:
			ADCCP = 0x00;
			ADCCN = 0x00;
			// Pseudo differential mode, negative ADC buffer bypassed
			reg_val |= BIT4 | BIT14;
			break;

		case padc1:
			ADCCP = BIT0;
			ADCCN = BIT0;
			// Pseudo differential mode, negative ADC buffer bypassed
			reg_val |= BIT4 | BIT14;
			break;
	}

	ADCCON = reg_val;
}

void adc_set_pga(adc channel, u8 gain)
{
	gain &= 0x1F;
	switch (channel) {	
		case padc0:
			PGAGN =	gain; 
			break;
		case padc1:
			PGAGN = (gain << 6);
			break;
		default:
			break;
	}
}

u16 adc_get_val()
{
	// Busy-wait for conversion completion
	while (ADCSTA == 0x00) {};
	return (ADCDAT >> 16);
}

// Continuous software conversion must be enabled
u16 adc_get_avg_val (const u16 num_samples)
{
	static u16 i;
	static u32 val;
	val = 0;
	for (i = 0; i < num_samples; i ++){
		while (ADCSTA == 0x00){};
		val += (ADCDAT >> 16);
	}
	return (u16)(val/num_samples);
}

// Continuous software conversion must be enabled
u16 adc_get_avgw_val (const u16 num_samples, u16 wait_time)
{
	u16 i, wait;
	u32 val = 0;
	for (i = 0; i < num_samples; i ++){
		wait = wait_time;		
		while (wait--);
		while (ADCSTA == 0x00){};
		val += (ADCDAT >> 16);		
	}
	return (u16)(val/num_samples);
}
