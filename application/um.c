#include "um.h"

#define DAC_2_V			2481
#define DAC_1_5_V		1861
#define DAC_1_V			1241

#define DAC_LIMIT_L		1540081

u16 scan_numpts;
extern u16 scan_l_points[1024];
extern u16 scan_r_points[1024];

struct um_peak
{
	u16 max_f;
	u16 iMax_f;
	u16 max_b;
	u16 iMax_b;
	u16 iMax;
};

struct umirror
{
	struct um_peak horz;
	struct um_peak vert;
};

struct umirror um = {{0, 0, 0}, {0, 0, 0}};

void um_init (void)
{
	adc_set_pga(ADC_MIRROR, 32);
}

#define DAC_HORZ	DAC_ZOFFSET_COARSE	
#define DAC_L1		DAC_X1
#define DAC_L2		DAC_Y1
#define UM_delay 	100
#define COM_delay 	100


void um_track (void)
{
	u16 val;
	u32 delay;
	s32 i;
	
	
	dac_set_limit(DAC_X1, DAC_1_V);
	dac_set_limit(DAC_Y1, DAC_1_V);
	dac_set_limit(DAC_Y2, 4095);
	//turn on photodiode
	dac_set_val(DAC_Y2, 4095);
	

	while (COMRX != 'q')
	{
		// Horizontal Tracking
		um.horz.max_f = 0;
		for (i = 0; i < 4095; i += 20)
		{
			delay = UM_delay;
			while (delay--);
			if (i > 4095) i = 4095;
			dac_set_val(DAC_HORZ, i);
			adc_start_conv(ADC_MIRROR);
			//val = adc_get_avgw_val(8, 100);
			val = adc_get_val();
			if (val > um.horz.max_f) 
			{
				um.horz.max_f = val;
				um.horz.iMax_f = i;
			}
		}
		um.horz.max_b = 0;
		delay = 1000;
		while(delay--);
		for (i = 4095; i > 0; i -= 20)
		{
			delay = UM_delay;
			while (delay--);
			if (i < 0) i = 0;
			dac_set_val(DAC_HORZ, i);
			adc_start_conv(ADC_MIRROR);
			//val = adc_get_avgw_val(8, 100);
			val = adc_get_val();
			if (val > um.horz.max_b) 
			{
				um.horz.max_b = val;
				um.horz.iMax_b = i;
			}
		}

		um.horz.iMax = (um.horz.iMax_f + um.horz.iMax_b) / 2;
		dac_set_val(DAC_HORZ, um.horz.iMax);

		delay = 1000;
		while(delay--){}
		// Vertical Tracking
		um.vert.max_f = 0;
		for (i = 0; i < scan_numpts; i++)
		{
			delay = UM_delay;
			while (delay--);

			dac_set_val(DAC_X1, scan_l_points[i]);
			dac_set_val(DAC_Y1, scan_r_points[i]);

			adc_start_conv(ADC_MIRROR);
			//val = adc_get_avgw_val(8, 100);
			val = adc_get_val();
			if (val > um.vert.max_f) 
			{
				um.vert.max_f = val;
				um.vert.iMax_f = i;
			}
		}
		
		um.vert.max_b = 0;
		for (i = scan_numpts; i > 0; i--)
		{
			delay = UM_delay;
			while (delay--); 

			dac_set_val(DAC_X1, scan_l_points[i]);
			dac_set_val(DAC_Y1, scan_r_points[i]);

			adc_start_conv(ADC_MIRROR);
			//val = adc_get_avgw_val(8, 100);
			val = adc_get_val();
			if (val > um.vert.max_b) 
			{
				um.vert.max_b = val;
				um.vert.iMax_b = i;
			}
		}

	//	um.vert.iMax = (um.vert.iMax_f + um.vert.iMax_b) / 2;
	  	um.vert.iMax = (um.vert.iMax_f + um.vert.iMax_b) / 2;

		dac_set_val(DAC_X1, scan_l_points[um.vert.iMax]);
		dac_set_val(DAC_Y1, scan_r_points[um.vert.iMax]);

		delay = 1000;
		while(delay--){}

		// set voltage to maximum
		// dac_set_val(DAC_VERT, dac_avg);

		uart_set_char (um.horz.iMax);
		uart_set_char (um.horz.iMax >> 8);

		uart_set_char (um.vert.iMax);
		uart_set_char (um.vert.iMax >> 8);	
		//delay = COM_delay;
		//while(delay--);
	}
}

