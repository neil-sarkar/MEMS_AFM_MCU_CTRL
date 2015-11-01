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
#define UM_delay 	1
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
static u16 range;
static u16 prevRange = 0;
static u16 hysteresis = 0;
static u16 threshold=2000;
static u16 prevMax=0;
static s32 DIR=1;
static u16 vertdata;

static const u16 edgeScanStepSize = 5;
static const u16 dacXScanMax = 4095;
static const u16 dacXScanMin = 1500;


void writeWordToUart(u16 wordToWrite)
{
	uart_set_char (wordToWrite);
	uart_set_char (wordToWrite >> 8);
}

void setPistonPosIdx(u16 posIdx)
{
	if (posIdx > scan_numpts) 	posIdx = scan_numpts;
	if (posIdx < 0) 						posIdx = 0;

	dac_set_val(DAC_X1, scan_r_points[posIdx]);
	dac_set_val(DAC_Y1, scan_l_points[posIdx]);
}

void edgeScanX()
{
	static bool isIncreasing=true;
	
	u16 initXVal, finalXVal;
	s16 stepSize;
	
	if (isIncreasing) {
		initXVal = dacXScanMin;
		stepSize = edgeScanStepSize;
		finalXVal = dacXScanMax;
	} else {
		initXVal = dacXScanMax;
		stepSize = -edgeScanStepSize;
		finalXVal = dacXScanMin;
	}
	
	for (i=initXVal; isIncreasing?i<finalXVal:i>finalXVal; i+= stepSize) {
			delay = UM_delay;
			while (delay--);
	
			dac_set_val(DAC_HORZ, i);
			adc_start_conv(ADC_MIRROR);
			val = adc_get_val();
		
			if (val > um.horz.iMax) {
				um.horz.iMax = val;
			}
			if (val < um.horz.iMin) {
				um.horz.iMin = val;
			}
			
			if (!triggered) {
				if (val>threshold) {
					threshold=threshold-hysteresis;
					triggered = true;
					trigpos[edgenum]=i;
					edgenum++;
				}
			}
			if (triggered) {	
				if (val<threshold) {
					threshold=threshold+hysteresis;
					triggered = false;
				}
 			}
	}
		
	isIncreasing = !isIncreasing;
}


void um_track (void)
{

	s32 vertpos = scan_numpts/2;
	
	//set pistons to midscale
	setPistonPosIdx(vertpos);
	
	while (exitFlag == 0)
	{
		// Horizontal Tracking
		um.horz.iMax = 0;
		um.horz.iMin = 4095;
		
		edgenum=0;

		edgeScanX();
		edgeScanX();
		
		// NZ: We are only using the data from the reverse sweep to set position
		um.horz.peak_posn = (trigpos[0] + trigpos[1]) / 2;
		um.horz.peak_posn = 4095 - um.horz.peak_posn; 
		
		range = um.horz.iMax - um.horz.iMin;
		threshold = um.horz.iMin + .7*range;
		hysteresis = range/10;
		
		// Recovery loop; sparse raster scan
		if((range<=400)&&(exitFlag==0)) {
			vertpos+=dir*(scan_numpts/8);
		
			if(vertpos>(scan_numpts-1)) {	
				dir=dir*-1;
				vertpos=scan_numpts-1;
			}

			if(vertpos<0) {	
				dir=dir*-1;
				vertpos=0;
			}
		
			iTerm=0;
	
			setPistonPosIdx(vertpos);
			
		} else {
			// Vertical tracking
			vertshift=(vertpos - prevVertPos);
			
			if (vertshift==0){
				vertshift=1;
			}
			
			// NZ: Why is this float?  Doesn't it just need to be signed?
			// Could go to fixed point if resolution is an issue
			
			fb = ((float)(range - prevRange)) / (vertshift);
			error = setpoint - fb;
			iTerm += (ki * error);
			
			if (iTerm >= scan_numpts/2) iTerm=scan_numpts/2;
			if (iTerm <= -1*scan_numpts/2) iTerm= -1*scan_numpts/2;
		
			pidval = (u16)(kp * error + iTerm);
			if (pidval==0) pidval=1;  //to do: make this direction dependant
		
			// Vertical Tracking
			//should we use range or max value to determine whether to switch directions?
			if (um.horz.iMax>prevMax) {
				if (dir>0) vertpos += pidval;
				if (dir<0) vertpos -= pidval;					
			} else {
				dir=dir*-1;
				if (dir>0) vertpos += pidval;
				if (dir<0) vertpos -= pidval; //when the pistons change directions, reset the iTerm?
			}				  

			if (vertpos > scan_numpts) 	vertpos = scan_numpts;
			if (vertpos < 0) 						vertpos = 0;
		
			setPistonPosIdx(vertpos);
		}
		delay = UM_delay;
		while (delay--);
		
		writeWordToUart(um.horz.peak_posn);		
		
		vertdata = vertpos*4095/scan_numpts; // scale from 0 to 3.3
		writeWordToUart(vertdata);
		
		prevMax			= um.horz.iMax;
		prevRange 	= range;
		prevVertPos = vertpos;
		
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





