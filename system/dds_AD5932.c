#include "dds_AD5932.h"

#include "../peripheral/uart.h"

#ifdef BOARD_m_assem
	#define DDS_DAT_REG		GP0DAT
	#define DDS_CTRL 		BIT17
	#define DDS_CTRL_DD		(DDS_CTRL<<8U)
#elif defined(BOARD_v2)
	#define DDS_DAT_REG		GP1DAT
	#define DDS_CTRL 		BIT22
	#define DDS_CTRL_DD		(DDS_CTRL<<8U)
#else 
	#error "DDS GPIO not defined"
#endif



// DDS registers
#define	REG_CTRL		0x0
#define REG_N_INCR		0x1
#define REG_FDELTA_L	0x2
#define REG_FDELTA_H	0x3
#define REG_T_INT		0x4
#define REG_FSTART_L	0xC
#define REG_FSTART_H	0xD

int dds_inc_cnt = 0;

unsigned char dds_data[14] = {
	// Write ctrl register 	
    0x0F,
	0xFF,
	
	// Write start frequency
	0xC2,
	0xFF,
	0xD0,
	0x00, 
	
	// Write frequency increment
	0x20,
	0xFF,
	0x30,
	0x00,
	
	// Write number of increments
	0x10,
	0x0F,
	
	// Write increment interval
	0x50,
	0x02
	};

volatile unsigned char writeCnt = 0;

void dds_spi_init()
{
	// Configure P0.2, P0.3,P0.4 and P0.5 for SPI mode
	GP0CON = BIT8 + BIT12 + BIT16 + BIT20;  // Select SPI alternative function
	
	// Configure P0.1 for DDS control, set to 0 and transition high
	DDS_DAT_REG |= DDS_CTRL_DD;
	DDS_DAT_REG &= ~DDS_CTRL;

	SPICON = BIT0 + BIT1
		   + BIT3
		   //+ BIT2						// CPOL = 0, CPHA = 1
		   //+ BIT5						// LSB first transfer
		   + BIT6						// Initiate transfer on write to Tx FIFO
		   + BIT11						// Continuous transfer mode
		   + BIT14;

	SPIDIV  = 0xC7;	   	         		// Select 101kHz clock rate
	IRQEN |= BIT14;

}

void dds_write() 
{
	// 1. Write to control register
	/* Control Register
	D0 = 1
	D1 = 1
	D2 = 1 SYNCOUT disabled
	D3 = 1
	D4 = 1
	D5 = 1 Frequency increments triggered ext
	D6 = 1
	D7 = 1
	D8 = 1 MSBOUT enabled
	D9 = 1 tri = 0, sine = 1
	D10 = 1 DAC enable
	D11 = 1 cont. write
	*/

	dds_inc_cnt = ((dds_data[10] & 0x0F) << 8) | dds_data[11];

   	writeCnt = 0;
	SPITX = dds_data[writeCnt++];
	SPITX = dds_data[writeCnt++];	
}

void dds_handler()
{
	if ((SPISTA & BIT5) == BIT5)    		// If SPI Master Tx IRQ
	{
		if (writeCnt == 14)
		{
			dds_increment();
			writeCnt = 0;			
		}
		else if ( writeCnt < 14)					// Have 14 bytes been sent?
		{
			SPITX = dds_data[writeCnt++];		// Load Tx buffer
			SPITX = dds_data[writeCnt++];		// Load Tx buffer
		}
	}		
}

void dds_increment()
{
	int cnt = 0xFF;
	DDS_DAT_REG &= ~DDS_CTRL;
	while (cnt--) {};
	DDS_DAT_REG |= DDS_CTRL;
}

void dds_get_data()
{
	u8 val_l, val_h;		  
			
	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	
	dds_data[2] = 0xC0 | (val_h & 0x0F);
	dds_data[3] = val_l;
	dds_data[5] = ((val_h & 0xF0) >> 4);
	
	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
		 
	dds_data[6] = 0x20 | (val_h & 0x0F); 
	dds_data[7] = val_l;

	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();

	dds_data[10] = 0x10 | (val_h & 0x0F);
	dds_data[11] = val_l;

	dds_write();
}
