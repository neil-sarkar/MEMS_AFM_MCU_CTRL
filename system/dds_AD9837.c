#include "dds_AD9837.h"

#include "../peripheral/uart.h"

// DDS registers
#define	REG_B28			0x2000

int dds_AD9837_inc_cnt = 0;
u16 cntrl_reg_data=REG_B28;
u32 freq =0;
u32 fstart = 0;
u32 increment = 0;
u16 curr_data = 0;

u8 dds_AD9837_data[12]={
	0x00,	 //FREQ0 LSB high
	0x00,	 //FREQ0 LSB low
	0x00,	 //FREQ0 MSB high
	0x00,	 //FREQ0 MSB low
	0x00,	 //Fstart LSB low for sweep
	0x00,	 //Fstart LSB high for sweep
	0x00,	 //Fstart MSB low for sweep
	0x00,	 //Fstart MSB high for sweep
	0x00,	 //increment LSB for sweep
	0x00,	 //increment MSB for sweep
	0x00,	 //numpts LSB in sweep
	0x00	 //numpts MSB in sweep
	};

u8 dds_9837_init[10] = 
{
   0x21,
   0x00,
   0x56,
   0xE0,
   0x40,
   0x10,      
   0xC0,
   0x00,
   0x20,
   0x00
};

volatile unsigned char d_writeCnt = 0;
volatile unsigned char d_writeLimit = 0;
volatile bool d_init = false;

void dds_AD9837_spi_init()
{
	// Configure P0.2, P0.3,P0.4 and P0.5 for SPI mode
	GP0CON |= BIT8 + BIT12 + BIT16 + BIT20;  // Select SPI alternative function

	SPICON = BIT0 + BIT1
		   + BIT3
		   + BIT6						// Initiate transfer on write to Tx FIFO
		   + BIT11						// Continuous transfer mode
		   + BIT14;

	SPIDIV  = 0xC7;	   	         		// Select 101kHz clock rate
	IRQEN |= BIT14;

	d_init 			= true;
	d_writeCnt 		= 0;
	d_writeLimit 	= 10;
  	SPITX = dds_9837_init[d_writeCnt++];
	SPITX = dds_9837_init[d_writeCnt++];
}

void dds_AD9837_write()
{
	d_writeCnt 		= 0;
	d_writeLimit 	= 4;
	d_init 			= false;
	SPITX = cntrl_reg_data >> 8;
	SPITX = cntrl_reg_data;
}
		
void dds_AD9837_get_data()
{
	u8 val_l, val_h;		  
			
	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	
	dds_AD9837_data[4] = val_l;		  //Fstart LSB
	dds_AD9837_data[5] = val_h; 
	
	val_l = uart_wait_get_char();	  
	val_h = uart_wait_get_char();
		 
	dds_AD9837_data[6] = val_l;		  //Fstart MSB
	dds_AD9837_data[7] = val_h;

	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();

	dds_AD9837_data[8] = val_l;		  //Increment
	dds_AD9837_data[9] = val_h;

	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
		 
	dds_AD9837_data[10] = val_l;	  //numpts
	dds_AD9837_data[11] = val_h;

	fstart = dds_AD9837_data[4] | (dds_AD9837_data[5] << 8) | (dds_AD9837_data[6] << 16) | (dds_AD9837_data[7] << 24);
	increment = dds_AD9837_data[8] | (dds_AD9837_data[9] << 8);
	dds_AD9837_inc_cnt = dds_AD9837_data[10] | (dds_AD9837_data[11] << 8);
	freq = fstart;
	dds_AD9837_load_freq();
	dds_AD9837_write();

}

void dds_AD9837_load_freq()		   //update the first four entries in dds_AD9837_data to the newest frequency
{
	dds_AD9837_data[0] = ((freq >> 8) & 0x3F) | 0x40; 
	dds_AD9837_data[1] = freq;
	dds_AD9837_data[2] = (((u8)(freq >> 22)) & 0x3F) | 0x40;
	dds_AD9837_data[3] = freq >> 14;	
}

void dds_AD9837_increment(u32 pt)
{
	freq = fstart + increment * pt;	 //new frequency equals the start frequency plus the frequency increment multiplied by the current pointin the frequency sweep
	
	dds_AD9837_load_freq();
	
	dds_AD9837_write();		
}

void dds_98_handler()
{
	if ((SPISTA & BIT5) == BIT5)    		// If SPI Master Tx IRQ
	{
		if (d_writeCnt == d_writeLimit)
		{
			d_writeCnt = 0;	
			d_init = false;		
		}
		else if ( d_writeCnt < d_writeLimit)	// Have 14 bytes been sent?
		{
			if (d_init == false)
			{
				SPITX =	dds_AD9837_data[d_writeCnt++];
				SPITX = dds_AD9837_data[d_writeCnt++];
			}
			else
			{
				SPITX = dds_9837_init[d_writeCnt++];
				SPITX = dds_9837_init[d_writeCnt++];
			}
		}
	}		
}
