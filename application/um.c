#include "um.h"

#define DAC_2_V			2481
#define DAC_1_5_V		1861
#define DAC_1_V			1241

#define DAC_LIMIT_L		1540081

u16 scan_numpts;
extern u16 scan_l_points[1024];
extern u16 scan_r_points[1024];
extern u8 exitFlag;

struct um_peak
{
	u16 max;
	u16 min;
	u16 iMax;
	u16 trigpos[2];
	u8 edgenum;
	bool triggered;
	u16 range;
	u16 hysteresis;
	u16 threshold;
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
	dac_set_limit(DAC_X1, DAC_1_V);
	dac_set_limit(DAC_Y1, DAC_1_V);
	dac_set_limit(DAC_Y2, 4095);
	//turn on photodiode
	dac_set_val(DAC_Y2, 4095);
}

#define DAC_HORZ	DAC_ZOFFSET_COARSE	
#define DAC_L1		DAC_X1
#define DAC_L2		DAC_Y1
#define UM_delay 	100
#define t_delay 	2000
#define COM_delay 	100

void um_track (void)
{
	u16 val;
	u32 delay;
	s32 i;
	u16 vertpos = scan_numpts/2;
	
	//set pistons to midscale
	dac_set_val(DAC_X1, scan_l_points[vertpos]);
	dac_set_val(DAC_Y1, scan_r_points[vertpos]);
	

	while (1)
	{
		// Horizontal Tracking
		um.horz.max = 0;
		um.horz.min = 4095;
		um.horz.edgenum=0;
		for (i = 0; i < 4095; i += 20)
		{
			if (i > 4095) i = 4095;
			dac_set_val(DAC_HORZ, i);

			if (i == 0)
			{
				delay = t_delay;
				while (delay--);
			}
			else
			{
				delay = UM_delay;
				while (delay--);
			}

			adc_start_conv(ADC_MIRROR);
			val = adc_get_val();
			
			if (val > um.horz.max) 
			{
				um.horz.max = val;
			}
			if (val < um.horz.min) 
			{
				um.horz.min = val;
			}
			if (!um.horz.triggered)
			{
				if (val>um.horz.threshold)
				{
					um.horz.threshold -= um.horz.hysteresis;
					um.horz.triggered = true;
					um.horz.trigpos[um.horz.edgenum]=i;
					um.horz.edgenum++;
				}
			}
			if (um.horz.triggered)
			{	
				if (val<um.horz.threshold)
				{
					um.horz.threshold += um.horz.hysteresis;
					um.horz.triggered = false;
				}
 			}
		}
		
		// TODO: are we sure about not resetting the min/max values for the reverse scan?
		for (i = 4095; i > 0; i -= 20)
		{
			delay = UM_delay;
			while (delay--);
			if (i < 0) i = 0;
			dac_set_val(DAC_HORZ, i);
			adc_start_conv(ADC_MIRROR);
			val = adc_get_val();
			if (val > um.horz.max) 
			{
				um.horz.max = val;
			}
			if (val < um.horz.min) 
			{
				um.horz.min = val;
			}
			if (!um.horz.triggered)
			{
				if (val>um.horz.threshold)
				{
					um.horz.threshold -= um.horz.hysteresis;
					um.horz.triggered = true;
					um.horz.trigpos[um.horz.edgenum]=i;
					um.horz.edgenum++;
				}
			}
			if (um.horz.triggered)
			{	
				if (val<um.horz.threshold)
				{
					um.horz.threshold += um.horz.hysteresis;
					um.horz.triggered = false;
				}
 			}
		}

		um.horz.iMax = (um.horz.trigpos[0] + um.horz.trigpos[1]) / 2;
		dac_set_val(DAC_HORZ, um.horz.iMax);

		um.horz.range = um.horz.max - um.horz.min;
		um.horz.threshold = um.horz.min + .5*um.horz.range;
		um.horz.hysteresis = um.horz.range/10;

		// Vertical Tracking
		um.vert.max = 0;
		um.vert.min = 4095;
		um.vert.edgenum = 0;
		for (i = 0; i < scan_numpts; i++)
		{	
			dac_set_val(DAC_X1, scan_r_points[i]);
			dac_set_val(DAC_Y1, scan_l_points[i]);

			if (i == 0)
			{
				delay = t_delay;
				while (delay--);
			}
			else
			{
				delay = UM_delay;
				while (delay--);
			}

			adc_start_conv(ADC_MIRROR);
			val = adc_get_val();
			
			if (val > um.vert.max) 
			{
				um.vert.max = val;
			}
			if (val < um.vert.min) 
			{
				um.vert.min = val;
			}
			if (!um.vert.triggered)
			{
				if (val > um.vert.threshold)
				{
					um.vert.threshold -= um.vert.hysteresis;
					um.vert.triggered = true;
					um.vert.trigpos[um.vert.edgenum] = i;
					um.vert.edgenum++;
				}
			}
			if (um.vert.triggered)
			{
				if (val < um.vert.threshold)
				{
					um.vert.threshold += um.vert.hysteresis;
					um.vert.triggered = false;
				}
			}
		}

		// TODO: are we sure about not resetting the min/max values for the reverse scan?		
		for (i = scan_numpts-1; i >= 0; i--)
		{ 
			delay = UM_delay;
			while (delay--);

			dac_set_val(DAC_X1, scan_r_points[i]);
			dac_set_val(DAC_Y1, scan_l_points[i]);
			
			adc_start_conv(ADC_MIRROR);
			val = adc_get_val();		

			if (val > um.vert.max) 
			{
				um.vert.max = val;
			}
			if (val < um.vert.min) 
			{
				um.vert.min = val;
			}
			if (!um.vert.triggered)
			{
				if (val > um.vert.threshold)
				{
					um.vert.threshold -= um.vert.hysteresis;
					um.vert.triggered = true;
					um.vert.trigpos[um.vert.edgenum] = i;
					um.vert.edgenum++;
				}
			}
			if (um.vert.triggered)
			{
				if (val < um.vert.threshold)
				{
					um.vert.threshold += um.vert.hysteresis;
					um.vert.triggered = false;
				}
			}
		}

		um.vert.iMax = (um.vert.trigpos[0] + um.vert.trigpos[1]) / 2;

		dac_set_val(DAC_X1, scan_r_points[um.vert.iMax]);
		dac_set_val(DAC_Y1, scan_l_points[um.vert.iMax]);

		um.vert.range = um.vert.max - um.vert.min;
		um.vert.threshold = um.vert.min + .8*um.vert.range;
		um.vert.hysteresis = um.vert.range/10;

		uart_set_char (um.horz.iMax);
		uart_set_char (um.horz.iMax >> 8);

		uart_set_char (scan_r_points[um.vert.iMax]);
		uart_set_char (scan_r_points[um.vert.iMax] >> 8);	
	}
	exitFlag = 0;
}

