#include "wire3.h"

#define WIRE3_DAT_REG GP3DAT

#define WIRE3_DATA BIT23
#define WIRE3_DATA_DD (WIRE3_DATA << 8U)

#define WIRE3_CLK BIT22
#define WIRE3_CLK_DD (WIRE3_CLK << 8U)

#define WIRE3_CLK_COUNTER 2

static volatile wire3_status status;

static u8 clk_cnt;
static u32 data_bfr;

struct wire3_config {
	bool	cpol;
	u8		clk_cnt;
	u32		cs_bit;
	u32		msb;
	volatile unsigned long * cs_dat_reg;
} wire3_config;

static void wire3_end_write (void);

void wire3_init (){
	// Controls clock signal speed
	T4LD = WIRE3_CLK_COUNTER;
	// Enable FIQs for timer 4
	FIQEN |= BIT6;
	// Set to periodic mode,
	// use higher frequency clock if signal needs to be faster
	T4CON |= BIT6;

	// Enable GPIO
	WIRE3_DAT_REG |= WIRE3_CLK_DD + WIRE3_DATA_DD;
	WIRE3_DAT_REG |= WIRE3_CLK + WIRE3_DATA;

	// Initialize state info to default values
	status = wire3_ok;
	wire3_config.cpol = true;
}

void wire3_set_cpol (bool cpol)
{
	wire3_config.cpol = cpol;
}

void wire3_set_cs (volatile unsigned long * cs_dat_reg, u32 cs_bit){
	wire3_config.cs_dat_reg = cs_dat_reg;
	wire3_config.cs_bit = cs_bit;
}

void wire3_write_wait (u32 data, u8 len){
	wire3_config.msb = 0x01<<(len*8-1);
	wire3_config.clk_cnt = len*8*2;
	data_bfr = data;

	// Set clock polarity
	if (wire3_config.cpol){
		WIRE3_DAT_REG |= WIRE3_CLK;
	} else {
		WIRE3_DAT_REG &= ~WIRE3_CLK;
	}
	// Pull chip select high, transition to low later
	(*wire3_config.cs_dat_reg) |= wire3_config.cs_bit;

	// Enable FIQ for timer and start timer
	T4CON |= BIT7;

	clk_cnt = 0;

	status = wire3_busy;

	while (status == wire3_busy);
	
	// Reset to active low signal
	WIRE3_DAT_REG |= WIRE3_CLK;
}

// Function should be optimized if attenuator switching happens more frequently
void wire3_handler (){
	// Because LM1971 requires that 1st CLK falling edge and CS happen within 20ns
	if (clk_cnt == 0){
		WIRE3_DAT_REG &= ~WIRE3_CLK;
		(*wire3_config.cs_dat_reg) &= ~wire3_config.cs_bit;
		
		// Sets data GPIO based on MSB in buffer
		WIRE3_DAT_REG ^= (-((data_bfr&wire3_config.msb)==wire3_config.msb)^WIRE3_DAT_REG)&WIRE3_DATA;
		data_bfr <<= 1;
		
		clk_cnt ++;
	}
	else if (clk_cnt < wire3_config.clk_cnt){
		// Toggles CLK line
		WIRE3_DAT_REG ^= WIRE3_CLK;

		// Ensures data is only toggled once every CLK cycle
		if (!(WIRE3_DAT_REG & WIRE3_CLK)){
			// Sets data GPIO based on MSB in buffer
			WIRE3_DAT_REG ^= (-((data_bfr&wire3_config.msb)==wire3_config.msb)^WIRE3_DAT_REG)&WIRE3_DATA;
			data_bfr <<= 1;
		}
		
		clk_cnt ++;
	} else {
		wire3_end_write ();
	}
	// Reset timer
	T4CLRI = 0x01;
}

static void wire3_end_write (void)
{
	// Finish last clock pulse
	WIRE3_DAT_REG ^= WIRE3_CLK;
	// Pull data to high
	(*wire3_config.cs_dat_reg) |= wire3_config.cs_bit;

	// Stop timer and timer FIQs
	T4CON &= ~BIT7;

	status = wire3_ok;
}

wire3_status get_status (void)
{
	return status;
}
