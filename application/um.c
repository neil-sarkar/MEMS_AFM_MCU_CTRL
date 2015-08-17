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
#define UM_delay 	50
#define COM_delay 	100

//pid variables
float ki = 0.01;
float kp = 0;

s32 iTerm;
s32 fb;
s32 error;
u16 pidval;
u16 setpoint = 0;
s32 vertshift;

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
	// TODO make this float?

	//set pistons to midscale
	dac_set_val(DAC_X1, scan_l_points[vertpos]);
	dac_set_val(DAC_Y1, scan_r_points[vertpos]);
	
	while (exitFlag == 0)
	{
		// Horizontal Tracking
		um.horz.iMax = 0;
		um.horz.iMin = 4095;
		edgenum=0;
		for (i = 0; i < 4095; i += 20)
		{
			delay = UM_delay;
			while (delay--);
			if (i > 4095) i = 4095;
			dac_set_val(DAC_HORZ, i);
			adc_start_conv(ADC_MIRROR);
			//val = adc_get_avgw_val(8, 100);
			val = adc_get_val();
			
			if (val > um.horz.iMax) 
			{
				um.horz.iMax = val;
			}
			if (val < um.horz.iMin) 
			{
				um.horz.iMin = val;
			}
			if (!triggered)
			{
				if (val>threshold)
				{
					threshold=threshold-hysteresis;
					triggered = true;
					trigpos[edgenum]=i;
					edgenum++;
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
//		um.horz.max_b = 0;
//		delay = 1000;
//		while(delay--);
		for (i = 4095; i > 0; i -= 20)
		{
			delay = UM_delay;
			while (delay--);
			if (i < 0) i = 0;
			dac_set_val(DAC_HORZ, i);
			adc_start_conv(ADC_MIRROR);
			//val = adc_get_avgw_val(8, 100);
			val = adc_get_val();
			if (val > um.horz.iMax) 
			{
				um.horz.iMax = val;
			}
			if (val < um.horz.iMin) 
			{
				um.horz.iMin = val;
			}
			if (!triggered)
			{
				if (val>threshold)
				{
					threshold=threshold-hysteresis;
					triggered = true;
					trigpos[edgenum]=i;
					edgenum++;
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

		um.horz.peak_posn = (trigpos[0] + trigpos[1]) / 2;
		//dac_set_val(DAC_HORZ, um.horz.peak_posn);
		
		range = um.horz.iMax - um.horz.iMin;
		threshold = um.horz.iMin + .5*range;
		hysteresis = range/10;
		vertshift=(vertpos - prevVertPos);
		if (vertshift==0)
		{
			vertshift=1;
		}
		fb = ((float)(range - prevRange)) / (vertshift);
		error = setpoint - fb;
		iTerm += (ki * error);
		
		if (iTerm > scan_numpts-1) iTerm = scan_numpts-1;
		else if (iTerm < 0) 			 iTerm = 0;
		
		pidval = (u16)(kp * error + iTerm);
		
		// Vertical Tracking
		if (um.horz.iMax>prevMax)
		{
			if (dir>0) vertpos += pidval;
			if (dir<0) vertpos -= pidval;					
		}
		else
		{
			dir=dir*-1;
			if (dir>0) vertpos += pidval;
			if (dir<0) vertpos -= pidval;				
		}				  

		if (vertpos > scan_numpts) 	vertpos = scan_numpts-1;
		if (vertpos < 0) 						vertpos = 0;	
	
		dac_set_val(DAC_X1, scan_r_points[vertpos]);
		dac_set_val(DAC_Y1, scan_l_points[vertpos]);
		
		// delay
		delay = UM_delay;
		while (delay--);

		// set voltage to maximum
		// dac_set_val(DAC_VERT, dac_avg);

		uart_set_char (um.horz.peak_posn);
		uart_set_char (um.horz.peak_posn >> 8);

		uart_set_char (vertpos);
		uart_set_char (vertpos >> 8);	
		
		prevMax			= um.horz.iMax;
		prevRange 	= range;
		prevVertPos = vertpos;
		
	}
	exitFlag = 0;
}

