#include "filter.h"

#include "../peripheral/uart.h"
#include "../peripheral/adc.h"

#define AVG_TMR_DFLT	4178

#define BUF_SIZE 1
volatile u16 filter_buf[BUF_SIZE];
volatile static u16 start_i = 0, end_i = 511;
volatile static u32 sum;

volatile u16 pid_input;
bool isAvgOn;

// in units if us
volatile static u16 sample_time = AVG_TMR_DFLT;
volatile static u16 sample_cnt = 512;

void filter_adc()
{
	// TODO pick ADC;

	u8 byte_l, byte_h;
	
	byte_l = uart_wait_get_char();
	byte_h = uart_wait_get_char();
	
	sample_time = (byte_h << 8) | byte_l;
	
	byte_l = uart_wait_get_char();
	byte_h = uart_wait_get_char();
	
	sample_cnt = (byte_h << 8) | byte_l;
	
	end_i = (start_i + sample_cnt) % BUF_SIZE;

}

void filter_enable(bool enable)
{
	if (enable)
	{
		// Initialize Timer1 for pid
		T2LD  = sample_time;
		// Periodic mode, core clock
		T2CON = BIT6 + BIT9;
		// Enable Timer2 fast interrupt
		FIQEN = BIT4;
		// Start clock
		T2CON |= BIT7;
	}
	else
	{
		T2CON &= ~BIT7;
	}
	
	isAvgOn = enable;
}

void filter_handler()
{
	//static int a = 1;

	//GP0DAT |= BIT23;

	end_i = ((end_i + 1) % BUF_SIZE);

	filter_buf[end_i] = adc_wait_get_reading(PID_INPUT);

	sum -= filter_buf[start_i];
	sum += filter_buf[end_i];

	start_i = ((start_i + 1) % BUF_SIZE);

	pid_input = sum / sample_cnt;
	//pid_input = 4095;

	//GP0DAT &= ~BIT23;					
}
