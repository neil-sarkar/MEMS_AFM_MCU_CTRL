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

typedef enum
{
	UP,
	DOWN
} RASTER_DIR;

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
#define UM_delay 	0
#define T_delay 0
#define COM_delay 	0

void um_raster (void)
{
	u16 val;
	u16 delay;
	s32 i;
	s16 vertpos = scan_numpts;
	bool triggered=false;
	u8 edgenum=0;
	u16 trigpos[2]={0,0};
	u16 range;
	u16 hysteresis=0;
	u16 threshold=2000;
	u16 globvertimax=0;
	u16 globhorzimax=0;
	u16 prevglobvertimax=0;
	u16 prevglobhorzimax=0;
	u16 horzsize=4095;
	u16 vertsize=scan_numpts;
	s16 lefthorz=0;
	u16 righthorz=4095;
	u16 topvert=scan_numpts;
	s16 botvert=0;
	u16 vert_step=1;
	
	RASTER_DIR dir = DOWN;
	
	//set pistons to top
	dac_set_val(DAC_X1, scan_l_points[topvert]);
	dac_set_val(DAC_Y1, scan_r_points[topvert]);
	
	// raster scan loop
	while (exitFlag == 0)
	{
		um.horz.iMax = 0;
		um.horz.iMin = 4095;
		edgenum=0;
		for (i = lefthorz; i < righthorz; i += 20)
		{
			delay = UM_delay;
			while (delay--);
			if (i > 4095) i = 4095;
			if (i < 0) i=0;
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
			if (!triggered)   /// make a global peak loop?
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
		
		for (i = righthorz; i > lefthorz; i -= 20)
		{
			delay = UM_delay;
			while (delay--);
			if (i > 4095) i=4095;
			if (i < 0) i=0;
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
			if (!triggered) //make a global peak loop?
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

		if(um.horz.iMax > globhorzimax)  // find global maximum intensity
			{
				globhorzimax = um.horz.iMax;
				globvertimax = vertpos;
				um.horz.peak_posn = (trigpos[0] + trigpos[1]) / 2;
			}
		//dac_set_val(DAC_HORZ, um.horz.peak_posn);
		
		range = um.horz.iMax - um.horz.iMin;
		threshold = um.horz.iMin + .5*range;
		hysteresis = range/10;
	
		// go to next vertical position - scan up and down - no sudden piston change
		// report peak posns at both top and bottom of scan.
		if (dir == DOWN)
		{
			if(vertpos >= scan_numpts) vertpos=scan_numpts-2;
			if(vertpos <= 0) vertpos=2;
			dac_set_val(DAC_X1, scan_r_points[vertpos-= vert_step]);
			dac_set_val(DAC_Y1, scan_l_points[vertpos-= vert_step]);
			delay = T_delay;
			while (delay--);
			if (vertpos <= botvert)
			{
				dir = UP;
		uart_set_char (um.horz.peak_posn);				// report horz peak posn
		uart_set_char (um.horz.peak_posn >> 8);
				
/*				if(globhorzimax >= prevglobhorzimax)	// adaptively scale scan size
				{
					horzsize = horzsize/2;
					if (horzsize <= 300)
					{
						horzsize = 300;
					}
					lefthorz = um.horz.peak_posn - horzsize/2;
					righthorz = um.horz.peak_posn + horzsize/2;
					if(lefthorz < 0) lefthorz=0;
					if(righthorz > 4095) righthorz=4095;
					
				}
				else
				{
				horzsize = horzsize*2;
					if (horzsize >=4095)
					{
						horzsize = 4095;
					}
					lefthorz = um.horz.peak_posn - horzsize/2;
					righthorz = um.horz.peak_posn + horzsize/2;
					if(lefthorz < 0) lefthorz=0;
					if(righthorz > 4095) righthorz=4095;
				}
			prevglobhorzimax=globhorzimax;
				globhorzimax=0;
*/
		uart_set_char (globvertimax);				// report verz peak posn
		uart_set_char (globvertimax >> 8);
				
				if(globhorzimax >= prevglobhorzimax)	// adaptively scale scan size
				{
					vertsize = vertsize/2;
					if (vertsize < 16)
					{
						vertsize = 16;
					}	
				}
				else
				{
				vertsize = vertsize*2;
					if (vertsize >scan_numpts)
					{
						vertsize = scan_numpts;
					}
				}
				vert_step = vertsize/16;
				topvert = globvertimax + vertsize/2;
				botvert = globvertimax - vertsize/2;
				if(botvert < 0) botvert=0;
				if(topvert > scan_numpts) topvert=scan_numpts;
				prevglobhorzimax=globhorzimax;
				globhorzimax=0;
				vertpos=botvert;
			dac_set_val(DAC_X1, scan_r_points[vertpos]);
			dac_set_val(DAC_Y1, scan_l_points[vertpos]);
				
			}
		}
		else   // i.e. DIR==UP but not first UP loop
		{
			if(vertpos >= scan_numpts) vertpos=scan_numpts-2;
			if(vertpos <= 0) vertpos=2;
			dac_set_val(DAC_X1, scan_r_points[vertpos+= vert_step]);
			dac_set_val(DAC_Y1, scan_l_points[vertpos+= vert_step]);
			delay = T_delay;
			while (delay--);
			if (vertpos >= topvert)
			{
				dir = DOWN;
		uart_set_char (um.horz.peak_posn);				//report horz peak posn
		uart_set_char (um.horz.peak_posn >> 8);
/*				if(globhorzimax >= prevglobhorzimax)	// adaptively scale scan size
				{
					horzsize = horzsize/2;
					if (horzsize <= 300)
					{
						horzsize = 300;
					}
					lefthorz = um.horz.peak_posn - horzsize/2;
					righthorz = um.horz.peak_posn + horzsize/2;
					if(lefthorz < 0) lefthorz=0;
					if(righthorz > 4095) righthorz=4095;
				}
				else
				{
				horzsize = horzsize*2;
					if (horzsize >=4095)
					{
						horzsize = 4095;
					}
					lefthorz = um.horz.peak_posn - horzsize/2;
					righthorz = um.horz.peak_posn + horzsize/2;
					if(lefthorz < 0) lefthorz=0;
					if(righthorz > 4095) righthorz=4095;
				}
				prevglobhorzimax=globhorzimax;
				globhorzimax=0;
*/
		uart_set_char (globvertimax);				// report vert peak posn
		uart_set_char (globvertimax >> 8);
				
		if(globhorzimax >= prevglobhorzimax)	// adaptively scale scan size
				{
					vertsize = vertsize/2;
					if (vertsize < 16)
					{
						vertsize = 16;
					}
				}
				else
				{
				vertsize = vertsize*2;
					if (vertsize > scan_numpts)
					{
						vertsize = scan_numpts;
					}
				}
				vert_step = vertsize/16;
				topvert = globvertimax + vertsize/2;
				botvert = globvertimax - vertsize/2;
				if(botvert < 0) botvert=0;
				if(topvert > scan_numpts) topvert=scan_numpts;
				prevglobhorzimax=globhorzimax;
				globhorzimax=0;
				vertpos=topvert;
			dac_set_val(DAC_X1, scan_r_points[vertpos]);
			dac_set_val(DAC_Y1, scan_l_points[vertpos]);
			}
		}
	}

	exitFlag = 0;	
}

void um_raster_step(void)
{
	u16 val;
	u32 delay;
	s32 i;
	u16 vertpos = scan_numpts;
	bool triggered=false;
	u8 edgenum=0;
	u16 trigpos[2]={0,0};
	u16 range;
	u16 hysteresis=0;
	u16 threshold=2000;

	RASTER_DIR dir = DOWN;
	
	//set pistons to midscale
	dac_set_val(DAC_X1, scan_l_points[vertpos]);
	dac_set_val(DAC_Y1, scan_r_points[vertpos]);
	
	// raster scan loop
	while (exitFlag == 0)
	{
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
	
		// go to next vertical position - scan up and down - no sudden piston change
		if (dir == DOWN)
		{
			dac_set_val(DAC_X1, scan_r_points[vertpos--]);
			dac_set_val(DAC_Y1, scan_l_points[vertpos--]);
			if (vertpos == 0)
			{
				dir = UP;
			}
		}
		else
		{
			dac_set_val(DAC_X1, scan_r_points[vertpos++]);
			dac_set_val(DAC_Y1, scan_l_points[vertpos++]);
			if (vertpos == scan_numpts)
			{
				dir = DOWN;
			}
		}
		
		// wait to scan next line or until user wants to exit
		while (uart_wait_get_char() != 'n' && exitFlag != 1) {}
		
	}

	exitFlag = 0;	
}

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
	exitFlag = 0;
}

