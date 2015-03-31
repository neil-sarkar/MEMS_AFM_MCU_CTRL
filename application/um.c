#include "um.h"
#include "math.h"

#define DAC_2_V			2481
#define DAC_1_5_V		1861
#define DAC_1_V			1241

#define DAC_LIMIT_L		1540081

u16 scan_numpts;
extern u16 scan_l_points[1024];
extern u16 scan_r_points[1024];

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
	s32 dir = 1;
	u16 vertpos = scan_numpts/2;
	bool triggered=false;
	u8 edgenum=0;
	u16 trigpos[2]={0,0};
	u16 range;
	u16 hysteresis=0;
	u16 threshold=2000;
	u16 prevmax=0;
	u16 serpstep=0;
	u8 xstate=0;
	s8 xdir=1;
	u16 xval=0;
	u16 yindex=0;
	u8 ystate=0;
	s8 ydir=1;
	u16 yval1=0;
	u16 yval2=0;
	u16 phase=scan_numpts/8+13;
	const u16 sinpts=128;
	float sintbl[sinpts];
	float sqrtsintbl[sinpts];
	float pi=3.14159265358979323846;
	u16 j=0;

	for (j = 0; j < (sinpts); j++)
		 {
		 sintbl[j]=0.5*(1+sin((float)j/sinpts*2*pi));
		 sqrtsintbl[j]=sqrt(sintbl[j]);
		 }


	
	dac_set_limit(DAC_X1, DAC_1_V);
	dac_set_limit(DAC_Y1, DAC_1_V);
	dac_set_limit(DAC_Y2, 4095);
	//turn on photodiode
	//dac_set_val(DAC_Y2, 4095);
	//set pistons to midscale
	//dac_set_val(DAC_X1, scan_l_points[vertpos]);
	//dac_set_val(DAC_Y1, scan_r_points[vertpos]);
	

	while (COMRX != 'q')
	{
		um.horz.iMax = 0;
		um.horz.iMin = 4095;
		edgenum=0;
		
		for (i = 0; i < (sinpts); i++)
		{
		//xstate=(2*i/(scan_numpts));
		//xdir=(xstate*-2+1);
		xval=3000*(sintbl[i]);
		
		dac_set_val(DAC_HORZ, xval);
		//write horz dac

		/*yindex=(i-phase) % scan_numpts;
		ystate=(yindex*4/scan_numpts) % 2;
		ydir=(ystate*-2+1);
		yval=ydir*(4*yindex % (scan_numpts))+((scan_numpts)*ystate);
		*/
		
		yval1=4000*(sqrtsintbl[(8*i)%sinpts]);
		yval2=4000*(sqrtsintbl[(8*i+64-20)%sinpts]);
		
		dac_set_val(DAC_X1, yval1);
		dac_set_val(DAC_Y1, yval2);
		
		
		delay = UM_delay;
		while (delay--);
		}
	}

		/*
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

		// Vertical Tracking
		if (um.horz.iMax>prevmax)
		{
			if (dir>0) vertpos++;
			if (dir<0) vertpos--;						
		}	
		else
		{
			dir=dir*-1;
			if (dir>0) vertpos++;
			if (dir<0) vertpos--;						
		}
						  

		if (vertpos>scan_numpts) vertpos=scan_numpts-1;
		//if (vertpos<0) vertpos=0;	
	
		dac_set_val(DAC_X1, scan_r_points[vertpos]);
		dac_set_val(DAC_Y1, scan_l_points[vertpos]);
		delay = UM_delay;
		while (delay--);
		prevmax=um.horz.iMax;
	

		// set voltage to maximum
		// dac_set_val(DAC_VERT, dac_avg);

		uart_set_char (um.horz.peak_posn);
		uart_set_char (um.horz.peak_posn >> 8);

		uart_set_char (vertpos);
		uart_set_char (vertpos >> 8);	
		}
		*/
	}

