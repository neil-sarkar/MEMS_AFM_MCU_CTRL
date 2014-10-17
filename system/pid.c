#include "pid.h"

#include "../peripheral/adc.h"
#include "../peripheral/dac.h"

/*
input: the sensed signal (ADC reading)
output: the output of the PID (DAC reading)
setPoint: reference signal (set manually)
*/

extern volatile u16 pid_input;
extern bool isAvgOn;

volatile u16 			input, 
						lastInput, 
						output, 
						setPoint = 0x7FF;
volatile int error;
volatile u16 dInput;
volatile int iTerm;
volatile float  kp = 1.6, 
				ki = 0, 
				kd = 0;

// Sample time is in ms for now (could be changed to us)
u16 sampleTime = 1;

volatile u16 outMin = 0, 
			   outMax = 3000;

static volatile bool pid_update_flag = false;

bool inAuto = false;

#define MICROSEC_CLK 41780
#define MS_TO_CLK(X) (X * MICROSEC_CLK)

void pid_set_p (float param)
{
	kp = param;
}

void pid_set_i (float param)
{
	// Receive paramter value as a single-precision floating point number (32-bit)
	float sampleTimeInSec = ((float)sampleTime / 1000);
	
	ki = param * sampleTimeInSec;
}

void pid_set_d (float param)
{
	// Receive paramter value as a single-precision floating point number (32-bit)
	float sampleTimeInSec = ((float)sampleTime / 1000);

	kd = param / sampleTimeInSec;
}

void pid_set_setpoint (u16 new_setpoint)
{
	setPoint = new_setpoint;
}

// TODO add capability to disable
void pid_enable(bool enable)
{
	if (enable)
	{
		// Initialize Timer1 for pid
		T1LD = MS_TO_CLK(sampleTime);
		// Periodic mode, core clock
		T1CON = BIT6 + BIT9;
		// Enable Timer1 fast interrupt
		FIQEN |= BIT3;
		// Start clock
		T1CON |= BIT7;

		outMax = dac_get_limit (PID_OUTPUT);
	}
	else
	{
		T1CON &= ~BIT7;
	}
}

// TODO get sampleTimeInSec or assume newSampleTime is in ms
void setSampleTime(u16 newSampleTime)
{
	if (newSampleTime > 0)
	{
		float ratio = (float)newSampleTime / (float)sampleTime;
		ki *= ratio;
		kd /= ratio;
		
		sampleTime = (u16)newSampleTime;
	}
}

void pid_handler(void)
{
	// Read ADC for input, busy-wait
	if (isAvgOn)
	{
		input = pid_input;
	}
	else
	{
		input = adc_wait_get_reading(PID_INPUT);
	}

	error = setPoint - input;
	iTerm += (ki * error);
	if (iTerm > outMax)
	{ 
		iTerm = outMax;
	}
	else if (iTerm < outMin)
	{
		iTerm = outMin;
	}
	dInput = (input - lastInput);

	output = kp * error + iTerm - kd * dInput;

	if (output > outMax) output = outMax;
	else if (output < outMin) output = outMin;

	// write PID output
	dac_set_val (PID_OUTPUT, output);

	lastInput = input;
	pid_update_flag = true;
}

void pid_wait_update (void)
{
	while (!pid_update_flag && (T1CON & BIT7));
	pid_update_flag = false;
}

/*
void setOutputLimits(float min, float max)
{
	if (min > max) return;
	outMin = min;
	outMax = max;

	if (output > outMax) output = outMax;
	else if (output < outMin) output = outMin;

	if (iTerm > outMax) iTerm = outMax;
	else if (iTerm < outMin) iTerm = outMin;
}

void setMode(pid_mode mode)
{
	bool newAuto = (bool)(mode == AUTO);
	if (newAuto && !inAuto)
	{
		initialize();
		// enable interrupt?
	}
	// else TODO disable interrupt?
	inAuto = newAuto;
}

void initialize()
{

	lastInput = input;
	iTerm = output;
	if (iTerm > outMax) iTerm = outMax;
	else if (iTerm < outMin) iTerm = outMin;
}

void compute()
{
	float error;
	float dInput;
	//if (!inAuto) return;

	error = setPoint - input;
	iTerm += (ki * error);
	if (iTerm > outMax) iTerm = outMax;
	else if (iTerm < outMin) iTerm = outMin;
	dInput = (input - lastInput);
	
	output = kp * error +
			 iTerm -
			 dInput;
	
	if (output > outMax) output = outMax;
	else if (output < outMin) output = outMin;
	lastInput = input;
}

*/
