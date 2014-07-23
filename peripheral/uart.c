#include "uart.h"

/* carriage return ASCII value */
#define CR     0x0D
#define BFR_SIZE 16

static s16 set_char (s16 ch);

static struct uart_fifo {
	u8 buffer [BFR_SIZE];
	u8 head;
	u8 tail;
	u8 num_bytes;
	uart_status status;
} rx_fifo;

void uart_init (void)
{

	// Initialize the UART for 9600-8-N
	
	/* Select UART functionality for P1.0/P1.1 */
	GP1CON = BIT0 | BIT4;     		  
	
	/* Enable access to COMDIV registers */
	COMCON0 = 0x080;
	
	/* Set baud rate to 9600 */
	//COMDIV0 = 0x88;
	/* Set baud rate to 19200 */
	//COMDIV0 = 0x44;
	/*Set baud rate to 76800 */
	COMDIV0 = 0x11;

	COMDIV1 = 0x00;
	
	/* Disable access to COMDIV registers */
	COMCON0 = 0x007;
}

void uart_buffered (bool enable)
{
	if (enable){
		COMIEN0 |= BIT0;
		FIQEN |= BIT13;
		rx_fifo.head = 0;
		rx_fifo.tail = 0;
		rx_fifo.num_bytes = 0;
		rx_fifo.status = UBFR_OK;
	} else {
		COMIEN0 &= ~BIT0;
		FIQEN &= ~BIT13;
	}
}

void uart_handler (void){
	unsigned long UARTSTATUS = COMIID0;

	if ((UARTSTATUS & (BIT2)) == (BIT2 & ~BIT1 & ~BIT0)){
		if (rx_fifo.num_bytes == BFR_SIZE){
			rx_fifo.status = UBFR_OVERFLOW;
		}
		rx_fifo.buffer [rx_fifo.tail] = COMRX;
		rx_fifo.tail = (rx_fifo.tail + 1)%BFR_SIZE;
		rx_fifo.num_bytes ++;
	}
}

/*
u8 uart_get_char (void)
{
	u8 data;
	if (rx_fifo.num_bytes <= 0){
		rx_fifo.status = UBFR_UNDERFLOW;
		return rx_fifo.buffer [rx_fifo.head];
	}
	data = rx_fifo.buffer [rx_fifo.head];
	rx_fifo.head = (rx_fifo.head + 1)%BFR_SIZE;
	rx_fifo.num_bytes --;
	return data;
}

u8 uart_wait_get_char (void)
{
	u8 data;
	while (rx_fifo.num_bytes <= 0);
	data = rx_fifo.buffer [rx_fifo.head];
	rx_fifo.head = (rx_fifo.head + 1)%BFR_SIZE;
	rx_fifo.num_bytes --;
	return data;
}

u8 uart_wait_get_bytes (void)
{
	u8 data;
} */

u8 uart_get_num_bytes (){
	return rx_fifo.num_bytes;
}

u32 uart_write (u8 *ptr) 
{
	u32 len = 0;

	for ( ; *ptr != '\0' ; len++) {
		set_char (*ptr++);
	}

	return len;
}

u32 uart_write_bytes (u8* ptr, u32 size)
{
	u32 len = size;
		if (*((u16*)(ptr)) > 4095)
		*((u16*)(ptr))=4095;
	while (size--)
	{
		while (!(0x020==(COMSTA0 & 0x020)));
		COMTX = *ptr++;
	}
	return (len - size);
}

// Non-blocking call
u8 uart_get_char (void)
{
	return (COMRX);
}

// Blocking call
u8 uart_wait_get_char (void)
{
	while (!((COMSTA0 & BIT0) == 0x01)) {}
	return (COMRX);
}

// Blocking call
u32 uart_wait_get_bytes (u8 * buffer, u32 num_bytes)
{
	u32 len = 0;
	while (len < num_bytes)
	{
		while (!((COMSTA0 & BIT0) == 0x01)) {}
		buffer[len++] = (COMRX);
	}
	return (len);
}

void uart_set_num(s16 num) 
{
	set_char(48+(num%10000)/1000);
	set_char(48+(num%1000)/100);
	set_char(48+(num%100)/10);
	set_char(48+num%10);
}

static s16 set_char (s16 ch)
{

	if (ch == '\n')  
	{
	
		while(!(0x020==(COMSTA0 & 0x020))) {};
		/*  output CR */
		COMTX = CR;
		
	}
    
	while(!(0x020==(COMSTA0 & 0x020))) {};
 
 	return (COMTX = ch);
	
}

void uart_set_char (u8 ch)
{
	while(!(0x020==(COMSTA0 & 0x020))) {};
 
 	COMTX = ch;	
}

bool is_received(void)
{

	return (bool)(COMSTA0 & 0x01);

}

void wait_receive(void)
{
	while (!((COMSTA0 & BIT0) == 0x01)) {}
}
