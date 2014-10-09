/***************************************************************************
This is the driver for controlling the AD5932 DDS chip.
Note that for writing the DDS registers, the MSB has to be sent out first.
***************************************************************************/

#include "dds.h"
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

// DDS register addresses
#define	REG_CTRL		(u8)0x0u
#define REG_N_INCR		(u8)0x1u
#define REG_FDELTA_L	(u8)0x2u
#define REG_FDELTA_H	(u8)0x3u
#define REG_T_INT		(u8)0x4u
#define REG_FSTART_L	(u8)0xCu
#define REG_FSTART_H	(u8)0xDu

// DDS 
#define DDS_MAX_ABS		(u32)16777215u
#define DDS_MCLK_HZ		(u32)25000000u
#define DDS_250HZ_ABS	(u8)168u
#define DDS_HZ_TO_ABS	(float)0.67f

// Macro helper functions
#define GET_BYTE_MS4B(val)	(u8)((val & 0xF0) >> 4)
#define GET_BYTE_LS4B(val)	(u8)(val & 0x0F)
#define GET_VAL(reg)		(u16)(((REG[reg].MSB & 0x0F) << 8) | REG[reg].LSB)						
#define GET_FREQ_VAL(reg_L, reg_H) \
							(u32)(((GET_VAL(reg_H)) << 12) | GET_VAL(reg_L)) 

extern void freq_sweep_dds(void);

// this is for improving this driver
typedef struct
{
	u8 addr;
	// most significant byte of the register value
	// note that the 4 most significant bits are the register address
	u8  MSB;
	// least significant byte of the register value   
	u8	LSB;
} DDS_REG;

// The order here MUST be maintained
typedef enum {
	// control register
	CTRL=0,

	// frequency start registers
	FSTART_L, 
	FSTART_H,

	// frequency increment (delta) register
	FDELTA_L,
	FDELTA_H,

	// frequency scan step count register
	N_INCR,

	// time delay register
	T_INT_MSB,
   	
	// total number of bytes in all registers
	REG_CNT
} REG_NAME;

volatile static u8 regCnt = 0;
u16 dds_inc_cnt = 0;

static DDS_REG REG[REG_CNT] = {
	// addr				   MSB	  	LSB
	{(REG_CTRL     << 4),  0x0F, 	0xFF},
	{(REG_FSTART_L << 4),  0x00, 	0x00},
	{(REG_FSTART_H << 4),  0x00, 	0x00},
	{(REG_FDELTA_L << 4),  0x00, 	0x00},
	{(REG_FDELTA_H << 4),  0x00, 	0x00},
	{(REG_N_INCR   << 4),  0x00, 	0x00},
	{(REG_T_INT    << 4),  0x00, 	0x00},
};

void dds_spi_init()
{
	// Configure P0.2, P0.3,P0.4 and P0.5 for SPI mode
	GP0CON = BIT8 + BIT12 + BIT16 + BIT20;  // Select SPI alternative function
	
	// Configure P0.1 for DDS CTRL input
	DDS_DAT_REG |= DDS_CTRL_DD;	 			// set direction to out
	DDS_DAT_REG &= ~DDS_CTRL;				// initialize to 0

   	// Configure SPI
	SPICON = BIT0 + BIT1
		   + BIT3
		   + BIT6							// Initiate transfer on write to Tx FIFO
		   + BIT11							// Continuous transfer mode
		   + BIT14;							// Tx interrupt occurs when 2-bytes have been transferred

	SPIDIV  = 0xC7;	   	         			// Select 101kHz clock rate
	IRQEN |= BIT14;							// Enable SPI interrupt
}

void dds_get_data()
{
	u8 val_l, val_h;
	u16 freq_hz;		  
	
	// data for frequency start		
	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();

	REG[FSTART_L].LSB 	= val_l;
	REG[FSTART_L].MSB  	= val_h;
	REG[FSTART_H].LSB	= GET_BYTE_MS4B(val_h);

//	dds_regs	
//	dds_data[2] = 0xC0 | (val_h & 0x0F);
//	dds_data[3] = val_l;
//	dds_data[5] = ((val_h & 0xF0) >> 4);

	// data for frequency increment	
	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	
	REG[FDELTA_L].LSB	= val_l;
	REG[FDELTA_L].MSB	= val_h; 
//	dds_data[6] = 0x20 | (val_h & 0x0F); 
//	dds_data[7] = val_l;

	// data for the number of scan steps
	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();

	REG[N_INCR].LSB		= val_l;
	REG[N_INCR].MSB		= val_h;
//	dds_data[10] = 0x10 | (val_h & 0x0F);
//	dds_data[11] = val_l;

	//dds_inc_cnt 		= ((val_h & 0x0F) << 8) | val_l;

	// configure the dds based on the new data
	dds_write();
}

void dds_get_all_data()
{
	u8 val_l, val_h, val_h2;
	u16 freq_hz;		  
	
	// data for frequency start		
	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	val_h2 = uart_wait_get_char();

	REG[FSTART_L].LSB 	= val_l;
	REG[FSTART_L].MSB  	= val_h;
	REG[FSTART_H].LSB	= (GET_BYTE_LS4B(val_h2) << 4) | GET_BYTE_MS4B(val_h);
	REG[FSTART_H].MSB	= GET_BYTE_MS4B(val_h2);

	// data for frequency increment	
	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	
	REG[FDELTA_L].LSB	= val_l;
	REG[FDELTA_L].MSB	= val_h; 

	// data for the number of scan steps
	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();

	REG[N_INCR].LSB		= val_l;
	REG[N_INCR].MSB		= val_h;

	// configure the dds based on the new data
	dds_write();
}

void dds_write() 
{
	// 1. Write to control register

	//TODO: what is this?
	//dds_inc_cnt = GET_BYTE_LS4B(REG[N_INCR].MSB) << 8 | REG[N_INCR].LSB;
	//dds_inc_cnt = (u16)((dds_data[10] & 0x0F) << 8) | dds_data[11];
	dds_inc_cnt = GET_VAL(N_INCR);

   	regCnt = 0;

	SPITX = REG[regCnt].addr | (REG[regCnt].MSB & 0x0F); 
	SPITX = REG[regCnt].LSB;
	regCnt++;	
}

void dds_increment()
{
	// TODO: Do we need a delay here? 
	// try the 0-1 transition without the delay and see if it works.
	u8 cnt = 0xFF;
	DDS_DAT_REG &= ~DDS_CTRL;
	while (cnt--) {};
	DDS_DAT_REG |= DDS_CTRL;
}

void dds_zoom()
{
	u32 current_f, start_f;
	current_f = GET_FREQ_VAL(FSTART_L, FSTART_H);
	start_f = current_f - DDS_250HZ_ABS;

	REG[FSTART_L].LSB = (u8)(start_f & 0xFF);
	REG[FSTART_L].MSB = (u8)((start_f & 0xFF00) >> 8);
	REG[FSTART_H].LSB = (u8)((start_f & 0xFF0000) >> 16);
	REG[FSTART_H].MSB = (u8)((start_f & 0xFF000000) >> 24);
   	
	REG[FDELTA_L].LSB = 0x02;
	REG[FDELTA_L].MSB = 0x00;
	REG[FDELTA_H].LSB = 0x00;
	REG[FDELTA_H].MSB = 0x00;

	REG[N_INCR].LSB	  = (u8)(DDS_250HZ_ABS >> 1);
	REG[N_INCR].MSB   = 0x00;

	freq_sweep_dds();
}

void dds_handler()
{
	if ((SPISTA & BIT5) == BIT5)    		// If SPI Master Tx IRQ
	{
		if ( regCnt < REG_CNT)				// Have 14 bytes been sent?
		{
			SPITX = REG[regCnt].addr | (REG[regCnt].MSB & 0x0F); 
			SPITX = REG[regCnt].LSB;
			regCnt++;
		}
		else if (regCnt == REG_CNT)
		{
			dds_increment();
			regCnt = 0;			
		}
	}		
}
