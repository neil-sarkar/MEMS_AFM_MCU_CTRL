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
	u16 iMax;
	u16 iMin;
	u16 edge_a;
	u16 edge_b;
	u16 peak_posn;
	u16 max;
	u16 min;
	u16 hysteresis;
	bool triggered;
	u16 threshold;
	u8 edgenum;
	u16 trigpos[2];
	u16 range;
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
#define COM_delay 	100

//pid variables
float ki = 0;
float kp = 1;

s32 iTerm;
s32 fb;
s32 error;
u16 pidval;
u16 setpoint = 0;

void um_set_i (float param)
{
	ki = param;
}

void um_set_p (float param)
{
	kp = param;
}

void um_track (void)
{
	u16 val;
	u32 delay;
	s32 i;
	s32 dir = 1;
	s32 vertpos = scan_numpts/2;
	u16 prevVertPos = 0;
	bool triggered=false;
	u8 edgenum=0;
	u16 trigpos[2] = {0,0};
	u16 range;
	u16 prevRange = 0;
	u16 hysteresis = 0;
	u16 threshold=2000;
	u16 prevMax=0;
	u16 horzpos=0;
	u16 prevhorzpos=2048;
	// TODO make this float?

	//set pistons to midscale
	dac_set_val(DAC_X1, scan_l_points[vertpos]);
	dac_set_val(DAC_Y1, scan_r_points[vertpos]);
	
	while (exitFlag == 0)
	{
				// Vertical Tracking
		um.vert.max = 0;
		um.vert.min = 4095;
		um.vert.edgenum = 0;
		for (i = 0; i < scan_numpts; i++)
		{	
			dac_set_val(DAC_X1, scan_r_points[i]);
			dac_set_val(DAC_Y1, scan_l_points[i]);

	/*		if (i == 0)
			{
				delay = t_delay;
				while (delay--);
			}
			else
			{
				delay = UM_delay;
				while (delay--);
			}*/
			
			delay = UM_delay;
				while (delay--);

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

	//	fb = ((float)(range - prevRange)) / (vertpos - prevVertPos);
		fb = ((float)(um.vert.max - prevMax)) / (horzpos - prevhorzpos);
		error = setpoint - fb;
		iTerm += (ki * error);
		
		if (iTerm > 4095) iTerm = 4095;
		else if (iTerm < 0) 			 iTerm = 0;
		
		pidval = kp * error + iTerm;
		
		// Horz Tracking
		if (um.vert.max>prevMax)
		{
			if (dir>0) horzpos += pidval;
			if (dir<0) horzpos -= pidval;					
		}
		else
		{
			dir=dir*-1;
			if (dir>0) horzpos += pidval;
			if (dir<0) horzpos -= pidval;				
		}				  

		if (horzpos > 4095) 	horzpos = 4095;
		if (horzpos < 0) 						horzpos = 0;	
	
		dac_set_val(DAC_HORZ, horzpos);
		
		
		// delay
		delay = UM_delay;
		while (delay--);

		// set voltage to maximum
		// dac_set_val(DAC_VERT, dac_avg);

		uart_set_char (um.vert.iMax);
		uart_set_char (um.vert.iMax>> 8);

		uart_set_char (horzpos);
		uart_set_char (horzpos >> 8);	
		
		prevMax			= um.vert.max;
		prevRange 	= range;
		prevhorzpos = horzpos;
		
	}
	exitFlag = 0;
}

