#include "i2c.h"
#include "uart.h"

void i2c_init(){
	   // Configure P0.0 and P0.1 for I2C mode
	GP0CON = BIT0 + BIT4;                   // Select I2C alternative function for P0.0 & P0.1
	
	// Enable I2C Master mode, baud rate and interrupt sources
	I2C0MCON = BIT0 + BIT4  			// Enable I2C Master + Enable Rx interrupt
			+ BIT5 + BIT8;  			// Enable Tx interrupt + Enable transmission complete interrupt
	I2C0DIV  = 0xCFCF;	   				// Select 100.3kHz clock rate
	IRQEN = BIT15; 				// Enable I2C Master interrupts
        //IRQCONE=BIT0+BIT1;          	// External IRQ0 triggers on falling edge
}

void i2c_handler(){
	if(I2C0MSTA & BIT3){
		u8 rx_char;
		rx_char = I2C0MRX;
		uart_set_char(rx_char);
	}
}

void i2c_send_read(){
	I2C0ADR0 |= BIT0;
	I2C0MTX = 0x08;
}

// Sets the COMTX register for sending UART
void i2c_write_char(u8 ch)
{ 
 	I2C0MTX = ch;	
}

u32 i2c_write_bytes (u8* ptr, u32 size)
{
	u32 len = size;
	
	if (*((u16*)(ptr)) > 4095)
		*((u16*)(ptr))=4095;
		
	while (size--)
	{
		//while (!(0x020==(I2C0MSTA & 0x020))); do we need this?
		I2C0MTX = *ptr++;
	}
	return (len - size);
}
