#include "../system/pga_i2c.h"

#define   CHP_ADDR 0x80;	//includes R/W bit set to Write

u16 cnt=65000;
u8 channel=1;					
u8 val=210;
u8 muteState=0;						

struct channels{
	u8 x1;
	u8 x2;
	u8 y1;
	u8 y2;
	u8 dds;
	u8 fine;
	u8 coarse;
	u8 level;
} channel_val;
void I2C_send(u8 arg)
{	
	I2C0MTX = arg;			 //Load the TX register with the data to be sent
	//I2C0MTX = val;
	//while ((I2C0MSTA & BIT2) != BIT2) {} //Wait until the TX FIFO	has space before continuing to write to it
}
void pga_init()
{		
	GP0CON = BIT0 + BIT4; // Configure P0.0 and P0.1 for I2C mode
	I2C0MCTL = BIT0;      // I2C master enable
	I2C0DIV = 0xCFCF;     // Select 100kHz clock rate
	send_address();		  //Write the chip address to the Address Register
	//GP0DAT &= ~BIT18;
	//while (cnt--) {};
	//GP0DAT |= BIT18;
}

void send_address()
{
	I2C0ADR0 = 	CHP_ADDR;	//Write the chip address to the Address Register
}

void mute(bool muteState)
{
	 //Set the GPIO connected to the MUTE pin as muteState

}	

void pga_get_data ()
{
	channel = uart_wait_get_char();
	val = uart_wait_get_char();

	switch (channel)
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
	
   //while ((I2C0MSTA & BIT10)==BIT10) {};

	I2C_send(channel);
	I2C_send(val);
}


