#include "main.h"

#include "../application/hex_file.h"
#include "../peripheral/uart.h"
#include "../peripheral/i2c.h"

#define MICROSEC_CLK 41780
#define MS_TO_CLK(X) (X * MICROSEC_CLK)

# include "stdio.h"
# include "string.h"

unsigned char szTxData[] = {0x1, 0x2, 0x10, 0x20, 0x40, 0x80};	// Array to send to Slave
unsigned char ucTxCount = 0;  			// Array index variable for szTxData[]
unsigned char szRxData[32];				// Array for reading Data from Slave
unsigned char ucRxCount = 0;  			// Array index variable for szRxData[]
unsigned char ucWaitingForXIRQ0 = 1; 	// Flag to begin reading from Slave - controlled by XIRQ0
unsigned char ucTxCountMax = 1;
unsigned char ucRxCountMax = 1;

bool isLEDOn = false;

int main(void)
{
	u8 rx_char;
	unsigned int i = 0;;
	
	/*
	 * MCU Initialization				  
	 */
		/* Configure CPU Clock for 41.78MHz, CD=0 */
	POWKEY1 = 0x01;
	POWCON  = 0x00;
	POWKEY2 = 0xF4;

	/* initialize all relevant modules */
	
	// Initialize Timer1 for 
	T1LD = MS_TO_CLK(1000);
	// Periodic mode, core clock
	T1CON = BIT6 + BIT9;
	// Enable Timer1 fast interrupt
	FIQEN |= BIT3;
	// Start clock
	T1CON |= BIT7;
	
	/* Initialize UART and I2C */
	uart_init();  
	i2c_init();
	
	//LED
	GP3CON = 0x00000000;
	GP3DAT = 0xFF000000;
	
	uart_set_char(0xFA);
	uart_set_char('a');
	uart_set_char('f');
	uart_set_char('m');
	uart_set_char('!');
	uart_set_char('\n');


	/*
	 * Main program loop
	 */
	while (true)
	{
		if (ucWaitingForXIRQ0 == 0)		// Wait for XIRQ0 to trigger - cleared in XIRQ0 ISR
		{
			ucWaitingForXIRQ0 = 1;
			ucRxCount = 0;
			I2C0MCNT0 = 24; 				// Read back 24 bytes
			I2C0ADR0 =  0x04 + 1;			// Begin Read sequence from Slave
			ucRxCountMax = 24;
		}
		if (ucRxCount == ucRxCountMax)					// Have max num of bytes been received?
		{
				for(i=0; i < ucRxCountMax; i++){ // Print to console
					uart_write_char(szRxData[i]);
				}
				ucRxCount = 0;
		}
		rx_char = "\n"; //Reset rx_char
		rx_char = uart_get_char();
		switch (rx_char)
		{
			// Set DAC
			case 'a':
				i2c_send_init();
				break;
			// Read DAC
			case 'b':
				i2c_send_test1();
				break;
			// Read ADC
			case 'c':
				//read_adc();
				break;
		}
	}
}

void i2c_send_test1(){
	// Begin Master Transmit sequence
	ucTxCountMax = 5;
  ucRxCountMax = 5;
	szTxData[0] = 0x02;
	ucTxCount = 0;
	I2C0FSTA = BIT9;					// Flush Master Tx FIFO
	I2C0FSTA &= ~BIT9;
	I2C0MTX = szTxData[ucTxCount++];
	I2C0ADR0 = 0x04; 				    // Write to the slave
	ucWaitingForXIRQ0 = 1;
	ucRxCount = 0;
}

void i2c_send_init(){
	uart_write("S");
	// Begin Master Transmit sequence
	ucTxCountMax = 1;
  ucRxCountMax = 1;
	szTxData[0] = 0x8;
	ucTxCount = 0;
	I2C0FSTA = BIT9;					// Flush Master Tx FIFO
	I2C0FSTA &= ~BIT9;
	I2C0MTX = szTxData[ucTxCount++];
	I2C0ADR0 = 0x04; 				    // Write to the slave
	ucWaitingForXIRQ0 = 1;
	ucRxCount = 0;
}

void do_something(){
	
	//i2c_send_test1();
	
	if(!isLEDOn){
		GP3CLR |= BIT19;
		isLEDOn=true;
	} else {
		GP3SET |= BIT19;
		isLEDOn=false;
	}
}

void IRQ_Handler(void)  __irq 
 {
	unsigned long IRQSTATUS = 0;
	unsigned int I2C0MSTATUS = 0;

	IRQSTATUS = IRQSTA;	   						// Read off IRQSTA register
	if ((IRQSTATUS & BIT19) == BIT19)			//XIRQ0 interrupt source
	{
		ucWaitingForXIRQ0 = 0;					// Enable Read request from Slave.
	        IRQCLR = BIT19;
        }

	if ((IRQSTATUS & BIT15) == BIT15)			//If I2C Master interrupt source
	{										
	   I2C0MSTATUS = I2C0MSTA;
	   if ((I2C0MSTATUS & BIT2) == BIT2)		// If I2C Master Tx IRQ
	   {
			if (ucTxCount < ucTxCountMax)					// Have max 6 bytes been sent?
				I2C0MTX = szTxData[ucTxCount++];// Load Tx buffer
	   }
	   if ((I2C0MSTATUS & BIT3) == BIT3) 		// If I2C Master Rx IRQ
	   {
			if (ucRxCount < ucRxCountMax)					// Have max 6 bytes been received?
			{
				szRxData[ucRxCount] = I2C0MRX;  // Read Rx buffer
				ucRxCount++;
			}
	   }
	}
}

void FIQ_Handler(void) __irq
{

	u32 FIQSTATUS = 0;

	FIQSTATUS = FIQSTA;

	if ((FIQSTATUS & BIT3) == BIT3) // Timer1 interrupt source - PID
	{
		do_something();
		T1CLRI = 0x55;				// Clear interrupt, reload T1LD
	}
	
		if ((FIQSTATUS & BIT13) == BIT13)
	{
		// Interrupt caused by hardware RX/TX buffer being full, cleared when
		// RX/TX buffer is read
		uart_handler();
	}
}
