#include "../system/pga_8ch_CS3308.h"

#ifdef configSYS_PGA_CS3308

#define   CHP_ADDR 0x80;	//includes R/W bit set to Write

u16 cnt=65000;
u8 channel=1;					
u8 val=1;
u8 muteState=0;	

struct channels{	   //Store the attenuation values for each channel in this struct
	u8 x1;
	u8 x2;
	u8 y1;
	u8 y2;
	u8 fine;
	u8 dds;
	u8 coarse;
	u8 level;
} channel_val;

void I2C_send(u8 channel, u8 val)
{	
	I2C0ADR0 = 	CHP_ADDR;	//Write the chip address to the Address Register
	I2C0MTX = channel;			 //Load the TX register with the data to be sent
	I2C0MTX = val;			 //Load the TX register with the data to be sent
}
void pga_init()
{		
	GP0CON |= BIT0 + BIT4; // Configure P0.0 and P0.1 for I2C mode
	I2C0MCON = BIT0;      // I2C master enable
	I2C0DIV = 0xCFCF;     // Select 100kHz clock rate
	I2C0ADR0 = 	CHP_ADDR;	//Write the chip address to the Address Register	 	
	GP3DAT |= BIT27; // Set as output p3.3 (MUTE)
	GP3DAT |= BIT26; // Set as output p3.2 (AD0)
	GP3DAT |= BIT24; // Set as output p3.0 (RESET)
	GP3DAT &= ~BIT18;  //Clear 3.2 (AD0)
	GP3DAT |= BIT16; //Set p3.0 HIGH (RESET)
	mute(1); 		 //Turn the mute off

	I2C0MTX = 0x0E;
	I2C0MTX = 0x00;
	//while ((I2C0MSTA & BIT0) != BIT0) {} //Wait until the TX FIFO	has space before continuing to write to it
	//I2C0MTX = 0x06;
	//I2C0MTX = 0xC6;
}

void mute(u8 muteState)
{	 
	 //Set the GPIO connected to the MUTE pin as muteState (p3.3, mute is active low)
	 if (muteState == 0)
	 	 {
	 	 GP3DAT &= ~BIT19;		//Clear p3.3, turning the mute on
		 }
		 else
		 {
		 GP3DAT |= BIT19;		//Set p3.3, turning the mute off
		 }
}	

void pga_get_data ()
{
	channel = uart_wait_get_char();		 //Get the chennel number that is goung to be set
	val = uart_wait_get_char();			 //Get the value (val=2*dB+210) that that will be written to channel

	switch (channel)					 //keep track of the current value of each channel as it's getting set
		{
			case (1):
				channel_val.x1 = val;
				break;
			case (2):
				channel_val.x2 = val;
				break;		
			case (3):
				channel_val.y1 = val;
				break;		
			case (4):
				channel_val.y2 = val;
				break;
			case (5):
				channel_val.dds = val;
				break;
			case (6):
				channel_val.fine = val;
				break;		
			case (7):
				channel_val.coarse = val;
				break;		
			case (8):
				channel_val.level = val;
				break;		
		}

	I2C_send(channel, val);				 //write the channel to the CS3308
}

#endif
