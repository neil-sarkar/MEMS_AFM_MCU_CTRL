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
#define UM_delay 	100
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

static u16 val;
static u32 delay;
static s32 i;
static s32 dir = 1;
static u16 prevVertPos = 0;
static bool triggered=false;
static u8 edgenum=0;
static u16 trigpos[2] = {0,0};
static u16 range=0;
static u16 prevRange = 0;
static u16 hysteresis = 0;
static u16 threshold=2000;
static u16 prevMax=0;
static s32 DIR=1;
static u16 vertdata;
static s32 peak_amp_val=0;
static s32 peak_phase_val=0;
	
void um_track (void)
{

	s32 vertpos = scan_numpts/2;
	
	//set pistons to midscale
	dac_set_val(DAC_X1, scan_l_points[vertpos]);
	dac_set_val(DAC_Y1, scan_r_points[vertpos]);
	
	while (exitFlag == 0)
	{
		// Horizontal Tracking
		um.horz.iMax = 0;
		um.horz.iMin = 4095;
		edgenum=0;
		peak_amp_val=0;
		
		for (i = 0; i < 64; i++)
		{
			vertpos+=DIR*(scan_numpts/64);
			if(vertpos>(scan_numpts-1))  
			{	
				DIR=DIR*-1;
				vertpos=scan_numpts-1;
			}
			if(vertpos<0) 
			{	
				DIR=DIR*-1;
				vertpos=0;
			}
			
			//if (vertpos > scan_numpts) 	vertpos = scan_numpts;
			//if (vertpos < 0) 						vertpos = 0;
			dac_set_val(DAC_X1, scan_r_points[vertpos]);
			dac_set_val(DAC_Y1, scan_l_points[vertpos]);
			delay = UM_delay;
			while (delay--);
			adc_start_conv(ADC_MIRROR);
			val = adc_get_val();
			if (val>peak_amp_val)
				
			{	peak_amp_val=val;
				vertdata = vertpos;
				adc_start_conv(ADC_PHASE);
				peak_phase_val = adc_get_val();
			}
		}
		
		um.horz.peak_posn=peak_phase_val;
		//vertdata=peak_amp_val;
		uart_set_char (um.horz.peak_posn);
		uart_set_char (um.horz.peak_posn >> 8);
		vertdata = (scan_numpts-vertdata)*4095/scan_numpts; // scale from 0 to 3.3
		uart_set_char (vertdata);
		uart_set_char (vertdata >> 8);
		
	}
	exitFlag = 0;
	
}


void printHex(u16 val)
{
	u8 d;
	u16 denom;
	for (denom = 1000; denom >= 1; denom/=10)
	{
		d = val/denom;
		uart_set_char(d+0x30);
		if (d > 0)
			val = val-d*denom;
	}
}


void um_genmap (u16 xpts, u16 ypts)
{
	u16 val;
	u32 delay;
	s32 i,j;
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
	s32 DIR=1;
	u16 vertdata;
	u32 d,denom;
	u16 horzVal;
	
	//u16 xscanpts=xpts;
	//u16 yscanpts=ypts;
	
	u16 xsteps=(float)((4095-1500)/xpts);
	u16 xmax=4095, xmin=1500;
	u16 stepsize=(xmax-xmin)/xpts;
	
	
	/*
	uart_set_char('S');
	uart_set_char('t');
	uart_set_char('a');
	uart_set_char('r');
	uart_set_char('t');
	uart_set_char('\r');
	*/
	
	vertpos = 0;
  // Set pistons to min-scale (vertpos initialized)	
	//dac_set_val(DAC_X1, scan_r_points[vertpos]);
	//dac_set_val(DAC_Y1, scan_l_points[vertpos]);	
	
	//delay = 10;
	//while (delay--);
	
	for (j = 0; j < ypts; j++) {
		dac_set_val(DAC_X1, scan_r_points[j]);
		dac_set_val(DAC_Y1, scan_l_points[j]);		
		
		/* i=1500;
		dac_set_val(DAC_HORZ, i);
		adc_start_conv(ADC_MIRROR);
		val = adc_get_val();
		printHex(val);
		*/
		
		for (i=0; i < xpts; i++) {
			//uart_set_char(',');
			//if (i > 4000) i = 4000;
			horzVal = xmin+stepsize*i;
			
			dac_set_val(DAC_HORZ, horzVal);
			adc_start_conv(ADC_MIRROR);
			val = adc_get_val();
			uart_set_char (val);
			uart_set_char (val >> 8);	
				delay = 50;
				while (delay--);
		}
		for (i=xpts; i > 0; i--) {
			//uart_set_char(',');
			//if (i > 4000) i = 4000;
			
			horzVal = xmax-stepsize*i;
			dac_set_val(DAC_HORZ, horzVal);
			
			adc_start_conv(ADC_MIRROR);
			val = adc_get_val();
			uart_set_char (val);
			uart_set_char (val >> 8);
				delay = 50;
				while (delay--);			
		}
		//uart_set_char('\r');
	}
}





