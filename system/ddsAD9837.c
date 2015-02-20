#include "ddsAD9837.h"

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
#define	REG_D15			0xF
#define REG_D14			0xE
#define	REG_B28			0x2000
#define REG_HLB			0xC
#define	REG_FSEL		0xB
#define REG_PSEL		0xA
#define REG_RESET		0x8
#define REG_SLEEP1		0x7
#define REG_SLEEP12		0x6
#define REG_OPBITEN		0x5
#define REG_DIV2		0x3
#define REG_MODE		0x1

u16 dds_AD9837_inc_cnt=0;
u16 cntrl_reg_data=REG_B28;
u32 freq =0;
u32 fstart = 0;
u32 increment = 0;
u16 curr_data = 0;

u8 dds_AD9837_data[12]={
	0x00,	 //FREQ0 LSB low
	0x00,	 //FREQ0 LSB high
	0x00,	 //FREQ0 MSB low
	0x00,	 //FREQ0 MSB high
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
   0x50,
   0xC7,
   0x40,
   0x00,      
   0xC0,
   0x00,
   0x20,
   0x00
};

void dds_AD9837_spi_init()
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

  	SPITX = dds_9837_init[writeCnt++];
	SPITX = dds_9837_init[writeCnt++];
}

void dds_AD9837_write()
{
	SPITX = cntrl_reg_data>>8;
	SPITX = cntrl_reg_data;

	SPITX = dds_AD9837_data[1] + 0x40;
 
	SPITX = dds_AD9837_data[0];

	SPITX = dds_AD9837_data[3] + 0x40;
 
	SPITX = dds_AD9837_data[2];
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

	fstart = dds_AD9837_data[4] + (dds_AD9837_data[5] << 8) + (dds_AD9837_data[6] << 16) + (dds_AD9837_data[7] << 24);
	increment = dds_AD9837_data[8] + (dds_AD9837_data[9] << 8);
	dds_AD9837_inc_cnt = dds_AD9837_data[10] + (dds_AD9837_data[11] << 8);
	freq = fstart;
}

void dds_AD9837_load_freq()
{
	dds_AD9837_data[0] = freq ; 
	dds_AD9837_data[1] = freq >>8;
	dds_AD9837_data[1] &= ~0xC0;
	dds_AD9837_data[2] = freq >>14;
	dds_AD9837_data[3] = freq >>22;	
}

void dds_AD9837_increment(u32 pt)
{
	freq = fstart + increment * pt;
	
	dds_AD9837_load_freq();
	
	dds_AD9837_write();		
}




