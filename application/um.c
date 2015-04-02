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
	u16 max;
	u16 iMax;
	u16 previMax;
	u16 min;
	u16 edge_a;
	u16 edge_b;
	u16 peak_posn;
	s32 deltaX;
	s32 deltaY1;
	s32 deltaY2;
	u16 delta_f;
	u16 phase_offset;
	u16 trigpos[1][1];
	u16 maxPeak;
	u16 corrDelta;
	u16 corrFactor;
};

struct umirror
{
	struct um_peak horz;
	struct um_peak vert;
};

//struct umirror um = {{0, 0, 0}, {0, 0, 0}};

struct um_peak um;// = {0, 0, 0, 0, 0, 0, 0, 0, 0, {0, 0, 0}};

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
	// TODO: need to set phase_offset to something

	u16 val;
	u32 delay;
	s32 i;
	s32 dir = 1;
	u16 vertpos = scan_numpts/2;
	bool triggered=false;
	u8 peakCnt=0;
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
	

	um.deltaX = 0;
	um.deltaY1 = 0;
	um.deltaY2 = 0;
	um.phase_offset = 0;
	while (uart_get_char() != 'q')
	{
		um.max = 0;
		um.min = 4095;
		peakCnt = 0;
		um.maxPeak = 0;	
		for (i = 0; i < (sinpts); i++)
		{
			delay = UM_delay;
			while (delay--);

			// calculate next point
			xval=3000*(sintbl[i] + um.deltaX);
			yval1=4000*(sqrtsintbl[(2*i)%sinpts] + um.deltaY1);
			yval2=4000*(sqrtsintbl[(2*i+64-20)%sinpts] + um.deltaY2);
			
			// write next point
			dac_set_val(DAC_HORZ, xval);
			dac_set_val(DAC_X1, yval1);
			dac_set_val(DAC_Y1, yval2);

			// read ADC			
			adc_start_conv(ADC_MIRROR);
			val = adc_get_val();

			if (val > um.max)
			{
				um.max = val;
			}
			if (val < um.min)
			{
				um.min = val;
			}

			if (!triggered)
			{
				if (val > threshold)
				{
					threshold = threshold - hysteresis;
					triggered = true;
					if (val == um.max)
					{ 
						um.iMax = (i - um.phase_offset + sinpts) % sinpts;
					}
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

		// calculate next point
		um.deltaX 	= sintbl[um.iMax] - sintbl[um.previMax];
		um.deltaY1 	= sqrtsintbl[(2*um.iMax)%sinpts] - sqrtsintbl[(2*um.previMax)%sinpts];
		um.deltaY2 	= sqrtsintbl[(2*um.iMax+64-20)%sinpts] - sqrtsintbl[(2*um.previMax+64-20)%sinpts];

		uart_set_char (um.iMax);
		uart_set_char (um.iMax >> 8);

		uart_set_char ((2*um.iMax)%sinpts);
		uart_set_char (((2*um.iMax)%sinpts) >> 8);

		um.previMax = um.iMax;
	}

	uart_reset_status ();
}
