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
	u16 max;
	u16 min;
	u16 iMax;
	u16 edge_a;
	u16 edge_b;
	u16 peak_posn;
	u16 trigpos[2];
	u8 edgenum;
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
#define UM_delay 	0
#define COM_delay 	100

void um_track (void)
{
	u16 val;
	u32 delay;
	s32 i;
	u16 vertpos = scan_numpts/2;
	bool triggered=false;
	u16 range;
	u16 hysteresis=0;
	u16 threshold=2000;
	
	dac_set_limit(DAC_X1, DAC_1_V);
	dac_set_limit(DAC_Y1, DAC_1_V);
	dac_set_limit(DAC_Y2, 4095);
	//turn on photodiode
	dac_set_val(DAC_Y2, 4095);
	//set pistons to midscale
	dac_set_val(DAC_X1, scan_l_points[vertpos]);
	dac_set_val(DAC_Y1, scan_r_points[vertpos]);
	

	while (COMRX != 'q')
	{
		// Horizontal Tracking
		um.horz.max = 0;
		um.horz.min = 4095;
		um.horz.edgenum=0;
		for (i = 0; i < 4095; i += 20)
		{
			delay = UM_delay;
			while (delay--);
			if (i > 4095) i = 4095;
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
			if (!triggered)
			{
				if (val>threshold)
				{
					threshold=threshold-hysteresis;
					triggered = true;
					um.horz.trigpos[um.horz.edgenum]=i;
					um.horz.edgenum++;
				}
			}
			if (triggered)
			{	
				if (val<threshold)
				{
					threshold=threshold+hysteresis;
					triggered = false;
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
			if (!triggered)
			{
				if (val>threshold)
				{
					threshold=threshold-hysteresis;
					triggered = true;
					um.horz.trigpos[um.horz.edgenum]=i;
					um.horz.edgenum++;
				}
			}
			if (triggered)
			{	
				if (val<threshold)
				{
					threshold=threshold+hysteresis;
					triggered = false;
				}
 			}
		}

		um.horz.iMax = (um.horz.trigpos[0] + um.horz.trigpos[1]) >> 1;
		dac_set_val(DAC_HORZ, um.horz.iMax);

		// Vertical Tracking
		um.vert.max = 0;
		um.vert.min = 4095;
		um.vert.edgenum = 0;
		for (i = 0; i < 4095; i += 20)
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
			if (!triggered)
			{
				if (val > threshold)
				{
					threshold = threshold-hysteresis;
					triggered = true;
					um.vert.trigpos[um.vert.edgenum] = i;
					um.vert.edgenum++;
				}
			}
			if (triggered)
			{
				if (val < threshold)
				{
					threshold = threshold + hysteresis;
					triggered = false;
				}
			}
		}

		// TODO: are we sure about not resetting the min/max values for the reverse scan?		
		for (i = 4095; i < 0; i -= 20)
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
			if (!triggered)
			{
				if (val > threshold)
				{
					threshold = threshold-hysteresis;
					triggered = true;
					um.vert.trigpos[um.vert.edgenum] = i;
					um.vert.edgenum++;
				}
			}
			if (triggered)
			{
				if (val < threshold)
				{
					threshold = threshold + hysteresis;
					triggered = false;
				}
			}
		}
			
		range = um.vert.max - um.vert.min;
		threshold = um.vert.min + .5*range;
		hysteresis = range/10;

		um.vert.iMax = (um.vert.trigpos[0] + um.vert.trigpos[1]) >> 1;

		dac_set_val(DAC_X1, scan_r_points[um.vert.iMax]);
		dac_set_val(DAC_Y1, scan_l_points[um.vert.iMax]);

		// TODO what is thi?
		// prevmax=um.horz.max;

		uart_set_char (um.horz.iMax);
		uart_set_char (um.horz.iMax >> 8);

		uart_set_char (um.vert.iMax);
		uart_set_char (um.vert.iMax >> 8);	
		}
	}

