#include "um.h"

#define DAC_2_V			2482
#define DAC_1_5_V		1861
#define DAC_1_V			1241

struct umirror
{
	u16 dac_val_f;
	u16 dac_val_b;
	u16 dac_val_min;
	u16 delta;
	u16 dac_delta;	
};

struct umirror um = {0, 0, 0, 10, 10};

void um_init (void)
{
	adc_set_pga(ADC_MIRROR, 32);
}

void um_set_delta (void)
{
	u8 val_l, val_h;

	val_l 			= uart_wait_get_char();
	val_h 			= uart_wait_get_char();
	um.delta 		= (val_h << 8) | val_l;

	val_l 			= uart_wait_get_char();
	val_h 			= uart_wait_get_char();
	um.dac_delta 	= (val_h << 8) | val_l;
}

void um_track (void)
{
	u16 val;
	u16 i;
	u16 max_f, max_b;
	u16 val_avg;
	
	while (COMRX != 'q')
	{
		max_f = 0;
		for (i = 0; i < DAC_2_V; i += 20)
		{
			dac_set_val(DAC_ZOFFSET_COARSE, i);
			adc_start_conv(ADC_MIRROR);
			//val = adc_get_avgw_val(8, 100);
			val = adc_get_val();
			if (val > max_f) 
			{
				max_f = val;
				um.dac_val_f = i;
			}
		}
		max_b = 0;
		for (i = DAC_2_V; i > 0; i -= 20)
		{
			dac_set_val(DAC_ZOFFSET_COARSE, i);
			adc_start_conv(ADC_MIRROR);
			//val = adc_get_avgw_val(8, 100);
			val = adc_get_val();
			if (val > max_b) 
			{
				max_b = val;
				um.dac_val_b = i;
			}			
		}
	
		// take average
		val_avg = ((um.dac_val_f + um.dac_val_b) >> 1);

		// set voltage to maximum
		//dac_set_val(DAC_ZOFFSET_COARSE, val_avg);

		uart_set_char (val_avg);
		uart_set_char (val_avg >> 8);

		uart_set_char (val);
		uart_set_char (val >> 8);	
	}
}

// TODO: remove
void um_sweep (void)
{
	u16 i;
	u16 val; 
	u16 max = 0;
	u16 min = 0xFFFF;

	// find maximum
	for (i = 0; i < DAC_1_V; i += 20)
	{
		dac_set_val(DAC_ZOFFSET_COARSE, i);
		adc_start_conv(ADC_MIRROR);
		val = adc_get_avgw_val(64, 100);
		if (val > max) 
		{
			max = val;
			um.dac_val_f = i;
		}
		if (val < min)
		{
			min = val;
		}
	}

	// set voltage to maximum
	dac_set_val(DAC_ZOFFSET_COARSE, um.dac_val_f);

	// send data
	uart_set_char(max);
	uart_set_char(max >> 8);

	uart_set_char(um.dac_val_f);
	uart_set_char(um.dac_val_f >> 8);
}

