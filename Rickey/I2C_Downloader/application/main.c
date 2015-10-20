#include "main.h"

#include "../application/hex_file.h"
#include "../peripheral/uart.h"
#include "../peripheral/i2c.h"

#define MICROSEC_CLK 41780
#define MS_TO_CLK(X) (X * MICROSEC_CLK)

# include "stdio.h"
# include "string.h"

unsigned char szTxData[255 + 2 + 1 + 1];	// 255 Data Bytes, 2 Start ID bytes, 1 Num of Bytes, and 1 Checksum byte
unsigned char TxDataBytes[255];
unsigned char ucTxCount = 0;  			// Array index variable for szTxData[]
unsigned char szRxData[32];				// Array for reading Data from Slave
unsigned char ucRxCount = 0;  			// Array index variable for szRxData[]
unsigned char ucWaitingForXIRQ0 = 1; 	// Flag to begin reading from Slave - controlled by UART message
unsigned char ucTxCountMax = 1;
unsigned char ucRxCountMax = 1;
unsigned long szAddr = 0x0;

bool isLEDOn = false;

int main(void)
{
	u8 rx_char, param1, param2;
	
	/*
	 * MCU Initialization				  
	 */
		/* Configure CPU Clock for 41.78MHz, CD=0 */
	POWKEY1 = 0x01;
	POWCON  = 0x00;
	POWKEY2 = 0xF4;

	/* initialize all relevant modules */
	
	// Initialize Timer1 for 
	T1LD = MS_TO_CLK(1111);
	// Periodic mode, core clock
	T1CON = BIT6 + BIT9;
	// Enable Timer1 fast interrupt
	IRQEN |= BIT3;
	// Start clock
	T1CON |= BIT7;
	
	/* Initialize UART and I2C */
	uart_init();  
	i2c_init();
	
	//LED
	GP3CON = 0x00000000;
	GP3DAT = 0xFF000000;
	
	uart_set_char('i');
	uart_set_char('!');
	uart_set_char('!');
	uart_set_char('!');
	uart_set_char('!');
	uart_set_char('\n');


	/*
	 * Main program loop
	 */
	while (true)
	{
		
		//if (ucWaitingForXIRQ0 == 0)		// Wait for XIRQ0 to trigger - cleared in XIRQ0 ISR
		//{
		//	ucWaitingForXIRQ0 = 1;
		//	uart_set_char(0xCC);
			/*
			ucRxCount = 0;
			I2C0MCNT0 = 24; 				// Read back 24 bytes
			I2C0ADR0 =  0x04 + 1;			// Begin Read sequence from Slave
			ucRxCountMax = 24;
			*/
		//}
		
		rx_char = uart_get_char_raw_no_memory();
		switch (rx_char)
		{
			// Send init message
			case 'i':
				i2c_send_init();
				break;
			//Set 32 bit address of target
			case 'a':
				set_address();
				break;
			// Request slave read
			case 'r':
				param1 = uart_wait_get_char_raw();
				if(param1 == '1'){
					request_slave_read(1);
				} else if (param1 == '2') {
					request_slave_read(24);
				}
				break;
			//Erase Memory for page X
			case 'e':
				param1 = uart_wait_get_char_raw();
				erase_memory(param1);
			//Go run
			case 'g':
				run_memory();
				break;
			//Cancel pending operation
			case 'c':
				uart_set_char('c');
				ucRxCountMax = 0;
				ucTxCountMax = 0;
				ucTxCount = 0; 
				ucRxCount = 0; 
				I2C0FSTA = BIT9;					// Flush Master Tx FIFO
				I2C0FSTA &= ~BIT9;
				I2C0FSTA = BIT8;					// Flush Master Rx FIFO
				I2C0FSTA &= ~BIT8;
				break;
		}
	
	}
}

void run_memory(){
	uart_set_char('g');
	TxDataBytes[0] = 'R';
	TxDataBytes[1] = 0x00;
	TxDataBytes[2] = 0x00;
	TxDataBytes[3] = 0x00;
	TxDataBytes[4] = 0x01;
//	i2c_send_data_bytes(5);
	
	szTxData[0] = 0x07;
	szTxData[1] = 0x0E;
	szTxData[2] = 0x05;
	szTxData[3] = 0x52;
	szTxData[4] = 0x00;
	szTxData[5] = 0x00;
	szTxData[6] = 0x00;
	szTxData[7] = 0x01;
	szTxData[8] = 0xA8;
		// Begin Master Transmit sequence
	ucTxCountMax = 5 + 2 + 1 + 1; //Start ID, Num Bytes, and Checksum
	ucTxCount = 0;
	I2C0FSTA = BIT9;					// Flush Master Tx FIFO
	I2C0FSTA &= ~BIT9;
	uart_set_char(szTxData[ucTxCount]);
	I2C0MTX = szTxData[ucTxCount++];
	I2C0ADR0 = 0x04; 				    // Write to the slave
}

void request_slave_read(int num_bytes) {
	uart_set_char('r');
	ucRxCount = 0; 
	ucRxCountMax = num_bytes;
	I2C0MCNT0 = num_bytes; 		// Read back X bytes
	I2C0ADR0 =  0x04 + 1;			// Begin Read sequence from Slave
}

void set_address() {
	//Collect four bytes to form the address
	TxDataBytes[1] = uart_wait_get_char_raw();
	uart_set_char(TxDataBytes[1]);
	TxDataBytes[2] = uart_wait_get_char_raw();
	uart_set_char(TxDataBytes[2]);
	TxDataBytes[3] = uart_wait_get_char_raw();
	uart_set_char(TxDataBytes[3]);
	TxDataBytes[4] = uart_wait_get_char_raw();
	uart_set_char(TxDataBytes[4]);
}

// Erase number of pages of memory starting at memory specified
void erase_memory(int pages){
	TxDataBytes[0] = 'E';
	TxDataBytes[5] = pages;
	i2c_send_data_bytes(6);
}

void print_i2c_read(){
	unsigned int i;
	for(i=0; i < ucRxCountMax; i++){ // Print to console
		uart_set_char(szRxData[i]);
	}
	return;
}

// Package the data bytes and send out.
// num_bytes should be between 5 and 255.
// The maximum number of data bytes allowed is 255:
// command function, 4-byte address, and 250 bytes of data.
void i2c_send_data_bytes(int num_bytes){
	int checksum, i;
	checksum = 0x00;
	
	//Construct the szTxData[] array
	//Start Byte
	szTxData[0] = 0x07;
	szTxData[1] = 0x0E;
	//Number of Data Bytes
	szTxData[2] = num_bytes;
	//Data Bytes and compute checksum
	for(i=0; i<num_bytes; i++){
		checksum = TxDataBytes[i] + checksum;
		szTxData[i + 3] = TxDataBytes[i];
	}
	//Compute Checksum
	//Subtract num of data bytes, then two's complement
	checksum = 0x00 - (num_bytes + checksum);
	szTxData[num_bytes + 3] = checksum;
	
	uart_set_char('W');
	//for(i=0; i<=(num_bytes+3); i++){

	//}
	
	// Begin Master Transmit sequence
	ucTxCountMax = num_bytes + 2 + 1 + 1; //Start ID, Num Bytes, and Checksum
	ucTxCount = 0;
	I2C0FSTA = BIT9;					// Flush Master Tx FIFO
	I2C0FSTA &= ~BIT9;
	uart_set_char(szTxData[ucTxCount]);
	I2C0MTX = szTxData[ucTxCount++];
	I2C0ADR0 = 0x04; 				    // Write to the slave
	
	// Don't forget to request acknowledge from loader
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
	uart_write("W");
	// Begin Master Transmit sequence
	ucTxCountMax = 1;
  ucRxCountMax = 1;
	szTxData[0] = 0x8;
	ucTxCount = 0;
	I2C0FSTA = BIT9;					// Flush Master Tx FIFO
	I2C0FSTA &= ~BIT9;
	uart_set_char(szTxData[ucTxCount]);
	I2C0MTX = szTxData[ucTxCount++];
	I2C0ADR0 = 0x04; 				    // Write to the slave
	ucWaitingForXIRQ0 = 1;
	ucRxCount = 0;
}

void do_something(){
	
	uart_set_char(I2C0FSTA);
	
					ucRxCountMax = 0;
				ucTxCountMax = 0;
				ucTxCount = 0; 
				ucRxCount = 0; 
				I2C0FSTA = BIT9;					// Flush Master Tx FIFO
				I2C0FSTA &= ~BIT9;
				I2C0FSTA = BIT8;					// Flush Master Rx FIFO
				I2C0FSTA &= ~BIT8;

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

	if ((IRQSTATUS & BIT15) == BIT15)			//If I2C Master interrupt source
	{										
	   I2C0MSTATUS = I2C0MSTA;
	   if ((I2C0MSTATUS & BIT2) == BIT2)		// If I2C Master Tx IRQ
	   {
			if (ucTxCount < ucTxCountMax)					// Have max bytes been sent?
				uart_set_char(szTxData[ucTxCount]);
				I2C0MTX = szTxData[ucTxCount++];// Load Tx buffer
	   }
	   if ((I2C0MSTATUS & BIT3) == BIT3) 		// If I2C Master Rx IRQ
	   {
			if (ucRxCount < ucRxCountMax)					// Have max bytes been received?
			{
				szRxData[ucRxCount] = I2C0MRX;  // Read Rx buffer
				uart_set_char(szRxData[ucRxCount]);
				ucRxCount++;
			}
	   }
		 if ((I2C0MSTATUS & BIT8) == BIT8){ // stop condition
			 uart_set_char('S');
		 }
	}
	
	if ((IRQSTATUS & BIT3) == BIT3) // Timer1 interrupt source - PID
	{
		do_something();
		T1CLRI = 0x55;				// Clear interrupt, reload T1LD
	}
	
	if ((IRQSTATUS & BIT13) == BIT13)
	{
		// Interrupt caused by hardware RX/TX buffer being full, cleared when
		// RX/TX buffer is read
		uart_handler();
	}
}

void FIQ_Handler(void) __irq
{

	u32 FIQSTATUS = 0;

	FIQSTATUS = FIQSTA;


}
