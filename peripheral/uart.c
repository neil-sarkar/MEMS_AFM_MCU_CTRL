#include "uart.h"

/* carriage return ASCII value */
#define CR     0x0D
#define BFR_SIZE 32

static s16 set_char (s16 ch);

static struct uart_fifo {
	u8 buffer [BFR_SIZE];
	u8 head;
	u8 tail;
	u8 num_bytes;
	volatile uart_status status;
} rx_fifo;

static u8 init_buffer (void);

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

	/* Enable uart interrupts */
	COMIEN0 |= BIT0 + BIT2;
	FIQEN |= BIT13;
	
	/* Initialize software buffer */
	init_buffer ();
}

void uart_handler (void){
	unsigned long UART_INTRPT = COMIID0;

	/* Checks if interrupt is set */
	if ((UART_INTRPT & (BIT0)) == 0)
	{
		/* Handle receive error interrupt */
		if ((UART_INTRPT & (BIT1 + BIT2)) == (BIT1 + BIT2))
		{
			if ((COMSTA0 & BIT1) == BIT1)
			{
				rx_fifo.status = UBFR_HW_OVERFLOW;
			}
			else 
			{
				rx_fifo.status = UART_RCV_ERROR;
			}
		} 
		/* Handle receive buffer full interrupt */
		else if ((UART_INTRPT & (BIT1 + BIT2)) == (BIT2 & (~BIT1)))
		{
			if (rx_fifo.num_bytes == BFR_SIZE)
			{
				/* Read COMRX to clear interrupt */
				rx_fifo.status = (uart_status)COMRX;
				rx_fifo.status = UBFR_OVERFLOW;
			}
			else
			{	
				/* Add byte to software uart buffer */
				rx_fifo.buffer [rx_fifo.tail] = COMRX;
				rx_fifo.tail = (rx_fifo.tail + 1)%BFR_SIZE;
				rx_fifo.num_bytes ++;
			}
			
		}
	}
}

u8 uart_get_num_bytes ()
{
	return rx_fifo.num_bytes;
}

uart_status uart_get_status (void)
{
	return rx_fifo.status;
}

u8 uart_reset_status (void)
{
	if (rx_fifo.status != UART_OK)
	{
		rx_fifo.status = UART_OK;
		return 0;
	}
	return 1;
}

/* Non-blocking call */
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

/* Blocking call */
u8 uart_wait_get_char (void)
{
	u8 data;
	while (rx_fifo.num_bytes <= 0);
	data = rx_fifo.buffer [rx_fifo.head];
	rx_fifo.head = (rx_fifo.head + 1)%BFR_SIZE;
	rx_fifo.num_bytes --;
	return data;
}

/* Blocking call */
u32 uart_wait_get_bytes (u8 * buffer, u32 num_bytes)
{
	u32 len = 0;
	while (len < num_bytes)
	{
		while (rx_fifo.num_bytes <= 0);
		buffer [len++] = rx_fifo.buffer [rx_fifo.head];
		rx_fifo.head = (rx_fifo.head + 1)%BFR_SIZE;
		rx_fifo.num_bytes --;
	}
	return (len);
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

void uart_set_char (u8 ch)
{
	while(!(0x020==(COMSTA0 & 0x020))) {};
 
 	COMTX = ch;	
}

bool is_received(void)
{

	return (bool)(rx_fifo.num_bytes > 0);

}

/* 	---------------------------
	Internal functions for UART 
	--------------------------- */
							
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

static u8 init_buffer (void)
{
	rx_fifo.head = 0;
	rx_fifo.tail = 0;
	rx_fifo.num_bytes = 0;
	rx_fifo.status = UART_OK;

	return 0;
}
