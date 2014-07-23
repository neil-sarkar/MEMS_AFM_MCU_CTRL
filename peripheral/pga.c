#include "pga.h"

struct pga{
 	unsigned char adr;
	unsigned int cs_dir_bit;
	unsigned int cs_bit;
	unsigned int data_buffer;
} pga;

void pga_set (unsigned char adr, unsigned int cs_dir_bit, u16 cs_bit){
	pga.adr = adr;
	pga.cs_dir_bit = cs_dir_bit;
	pga.cs_bit = cs_bit;

	GP3DAT |= pga.cs_dir_bit;
	GP3DAT |= pga.cs_bit;
}

void wire3_init (){
	// Controls clock signal frequency
	T4LD = WIRE3_CLK_CNT;
	// Enable IRQs for timer 4
	IRQEN |= BIT6;
	// Set to periodic mode,
	// use higher frequency clock if signal needs to be faster
	T4CON |= BIT6;
	
	// Sets WIRE3
	GP3DAT |= WIRE3_DATA_DIR
		+ WIRE3_CLK_DIR;

	GP3DAT |= WIRE3_CLK + WIRE3_DATA;
}

void wire3_write (unsigned char db){
	// Pull clock and data to high
	GP3DAT |= pga.cs_bit + WIRE3_CLK;

	// Enable IRQ for timer and start timer
	IRQEN |= BIT6;
	T4CON |= BIT7;

	// Init output buffer
	pga.data_buffer = (db) + (pga.adr << 8);

	bit_cnt = 0;
}

void wire3_handler (){
	// Function should be optimized if attenuator switching happens more frequently
	unsigned int bit;
	// Because LM1971 prefers that 1st CLK falling edge and CS happen within 20ns
	if (bit_cnt == 0){
		if (!(bit_cnt % 2)){
			bit = (pga.data_buffer&0x8000);
			pga.data_buffer = pga.data_buffer << 1;
			if (bit)
				GP3DAT |= WIRE3_DATA;
			else
				GP3DAT &= ~WIRE3_DATA;
		}
		GP3DAT &= ~(pga.cs_bit + WIRE3_CLK);
		bit_cnt = 1;
		T4CLRI = 0x55;
	}
	else if (bit_cnt < 32){
		// Ensures data is only toggled once every CLK cycle
		if (!(bit_cnt % 2)){
			// Needed to change output from MSB to LSB
			bit = (pga.data_buffer&0x8000);
			// Sets data GPIO based on MSB in buffer
			pga.data_buffer = pga.data_buffer << 1;
			if (bit)
				GP3DAT |= WIRE3_DATA;
			else
				GP3DAT &= ~WIRE3_DATA;
		}
		// Toggles CLK line
		GP3DAT ^= WIRE3_CLK;
		bit_cnt ++;
		// Reset timer
		T4CLRI = 0x55;
	} else {
		wire3_end_write ();
	}
}

void wire3_end_write (){
	// Pull clock and data to high
	GP3DAT |= (pga.cs_bit + WIRE3_CLK);

	// Stop timer and timer IRQs
	IRQCLR |= BIT6;
	T4CON &= ~BIT7;
}
