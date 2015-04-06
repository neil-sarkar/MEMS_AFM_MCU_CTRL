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
	u16 min;
	u16 iMin;
	s32 deltaX;
	s32 deltaY1;
	s32 deltaY2;
	u16 phaseOffset;
	u16 trigpos[4][2];
	u16 maxPeak;
	u16 corrDelta;
	u16 corrFactor;
	u16 maxPos;
	s16 offsetX;
	s16 offsetY1;
	s16 offsetY2;
};

struct um_peak um;

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
#define SINPTS		64

void um_track (void)
{
	u16 val;
	u32 delay;
	s32 i;

	bool triggered=false;
	u8 peakCnt=0;
	u16 range;
	u16 hysteresis=0;
	u16 threshold=2000;

	u16 xval=0;
	u16 yval1=0;
	u16 yval2=0;

	float sintbl[SINPTS];
	float sqrtsintbl[SINPTS];
	float pi=3.14159;//265358979323846;

	for (i = 0; i < (SINPTS); i++)
	{
		sintbl[i]=0.5*(1+sin((float)i/SINPTS*2*pi));
		sqrtsintbl[i]=sqrt(sintbl[i]);
	}

	um.deltaX = 0;
	um.deltaY1 = 0;
	um.deltaY2 = 0;
	um.phaseOffset = 0;
	um.offsetX = 0;
	um.offsetY1 = 0;
	um.offsetY2 = 0;
	while (uart_get_char() != 'q')
	{
		um.max = 0;
		um.min = 4095;
		peakCnt = 0;
		um.maxPeak = 0;	
		for (i = 0; i < SINPTS; i++)
		{
			delay = UM_delay;
			while (delay--);

			// calculate next point
			xval=2000*(sintbl[i]) + um.offsetX;
			yval1=2000*(sqrtsintbl[(2*i)%SINPTS]) + um.offsetY1;
			yval2=2000*(sqrtsintbl[(2*i+32)%SINPTS]) - um.offsetY2;
			
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
				um.iMax = (i - um.phaseOffset + SINPTS) % SINPTS;
			}
			if (val < um.min)
			{
				um.min = val;
				um.iMin = (i - um.phaseOffset + SINPTS) % SINPTS;
			}

			if (!triggered)
			{
				if (val > threshold)
				{
					threshold = threshold - hysteresis;
					triggered = true;
					um.trigpos[peakCnt][0] = (i - um.phaseOffset + SINPTS) % SINPTS;
				}
			}
			if (triggered)
			{
				if (val < threshold)
				{
					threshold = threshold + hysteresis;
					triggered = false;
					um.trigpos[peakCnt][1] = (i - um.phaseOffset + SINPTS) % SINPTS;
					peakCnt++;	
				}
			}
		}

	   	for (i = 0; i < peakCnt; i++)
		{
			if (um.trigpos[i][0] < um.iMax && um.trigpos[i][1] > um.iMax)
				break;
		}

		range = um.max - um.min;
		threshold = um.min + 0.8*range;
		hysteresis = range/10;

		um.maxPos = (um.trigpos[i][0] + um.trigpos[i][1]) / 2;

		// calculate next point
		um.deltaX = 500*(sintbl[um.maxPos] - sintbl[0]);
		um.deltaY1 = 500*(sqrtsintbl[(2*um.maxPos)%SINPTS] - sqrtsintbl[0]);
		um.deltaY2 = 500*(sqrtsintbl[(2*um.maxPos+32)%SINPTS] - sqrtsintbl[0]);

		um.offsetX += um.deltaX;
		um.offsetY1 += um.deltaY1;
		um.offsetY2 += um.deltaY2;

		if (um.offsetX > 2095) um.offsetX = 2095;
		if (um.offsetY1 > 2095) um.offsetY1 = 2095;
		if (um.offsetY2 > 2095) um.offsetY2 = 2095;
		
		if (um.offsetX < 0) um.offsetX = 0;
		if (um.offsetY1 < 0) um.offsetY1 = 0;
		if (um.offsetY2 < 0) um.offsetY2 = 0;
				  
		uart_set_char (um.offsetX);
		uart_set_char (um.offsetX >> 8);

		uart_set_char (um.offsetY1);
		uart_set_char (um.offsetY1 >> 8);
	}

	uart_reset_status ();
}
