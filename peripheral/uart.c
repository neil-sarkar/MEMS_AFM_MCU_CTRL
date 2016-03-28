#include "uart.h"

#define SERIAL_MSG_NEWLINE 0x0A
#define SERIAL_MSG_ESCAPE 0x10
#define SERIAL_MSG_MASK 0x80

/* carriage return ASCII value */
#define LF     0x0A
#define CR     0x0D
#define BFR_SIZE 128

typedef enum
{
	NO_RESET = 0,
	LAYER1,
	LAYER2,
	LAYER3,
	LAYER4
} RST_STATES;

#define RESET_INITIATOR_CHAR	'M'
#define LAYER1_RESET_CHAR			'A'
#define LAYER2_RESET_CHAR			'B'
#define LAYER3_RESET_CHAR			'C'
#define LAYER4_RESET_CHAR			'D'

bool buffer_full = false;

RST_STATES rst_state = NO_RESET;

#define RESET_SEQUENCE()			if (rx_fifo.rx == RESET_INITIATOR_CHAR && rst_state == NO_RESET) 	\
															{ 																																\
																rst_state = LAYER1; 																						\
															}																																	\
															else if (rst_state != NO_RESET)																		\
															{																																	\
																switch (rst_state)																							\
																{																																\
																	case LAYER1:																									\
																		if (rx_fifo.rx == LAYER1_RESET_CHAR)												\
																			rst_state = LAYER2;																				\
																		else																												\
																			rst_state = NO_RESET;																			\
																		break;																											\
																	case LAYER2:																									\
																		if (rx_fifo.rx == LAYER2_RESET_CHAR)												\
																			rst_state = LAYER3;																				\
																		else																												\
																			rst_state = NO_RESET;																			\
																		break;																											\
																	case LAYER3:																									\
																		if (rx_fifo.rx == LAYER3_RESET_CHAR)												\
																			rst_state = LAYER4;																				\
																		else																												\
																			rst_state = NO_RESET;																			\
																		break;																											\
																	case LAYER4:																									\
																		rst_state = NO_RESET;																				\
																		if (rx_fifo.rx == LAYER4_RESET_CHAR)												\
																			RSTSTA |= BIT2;																						\
																		break;																											\
																	default:																											\
																		rst_state = NO_RESET;																				\
																		break;																											\
																}																																\
															}																																	\

// Gloabl reset variables
#define 							G_EXIT_CHAR 		'q'
volatile char 				G_EXIT_FLAG 		= 0;
															
static struct uart_fifo {
	u8 buffer [BFR_SIZE];
	u8 head;
	u8 tail;
	u8 rx;
	volatile u8 num_bytes;
	volatile uart_status status;
} rx_fifo;

static s16 set_char (s16 ch);
static u8 init_buffer (void);

extern void reset_mcu (void);

void uart_init (void)
{

	// Initialize the UART for 9600-8-N
	
	/* Select UART functionality for P1.0/P1.1 */
	GP1CON |= BIT0 | BIT4;     		  
	
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
			rx_fifo.rx = COMRX;
			if (rx_fifo.num_bytes == BFR_SIZE)
			{
				/* Read COMRX to clear interrupt */
				rx_fifo.status = (uart_status)rx_fifo.rx;
				rx_fifo.status = UBFR_OVERFLOW;
				buffer_full = true;
			}
			else
			{	
				/* Add byte to software uart buffer */
				rx_fifo.buffer [rx_fifo.tail] = rx_fifo.rx;
				rx_fifo.tail = (rx_fifo.tail + 1)%BFR_SIZE;
				rx_fifo.num_bytes ++;
			}

			RESET_SEQUENCE();
			
			if (rx_fifo.rx == G_EXIT_CHAR)
			{
				G_EXIT_FLAG = 1;
			}
		}
	}
}

u8 uart_get_num_bytes (void)
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
// Obtains the raw char
u8 uart_get_char_raw (void)
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

/* Non-blocking call */
// Obtains the unmasked char
u8 uart_get_char (void)
{
	u8 rx_char = uart_get_char_raw();
	
	if(rx_char == SERIAL_MSG_ESCAPE){
		rx_char = uart_get_char_raw();
		return rx_char & ~SERIAL_MSG_MASK;
	} else {
		return rx_char;
	}
}

/* Blocking call */
// Obtains the raw character
u8 uart_wait_get_char_raw (void)
{
	u8 data;
	while (rx_fifo.num_bytes <= 0);
	data = rx_fifo.buffer [rx_fifo.head];
	rx_fifo.head = (rx_fifo.head + 1)%BFR_SIZE;
	rx_fifo.num_bytes --;
	return data;
}

/* Blocking call */
// Obtains the unmasked character
u8 uart_wait_get_char (void)
{
	u8 rx_char = uart_wait_get_char_raw();
	
	if(rx_char == SERIAL_MSG_ESCAPE){
		rx_char = uart_wait_get_char_raw();
		return rx_char & ~SERIAL_MSG_MASK;
	} else {
		return rx_char;
	}
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


/* Writes a raw byte (no masking or escaping) */
u32 uart_write_raw (u8 *ptr) 
{
	u32 len = 0;

	for ( ; *ptr != '\0' ; len++) {
		set_char (*ptr++);
	}

	return len;
}


/*
Masks appropriate bytes before sending it out to UART
Can be used for all functions that writes to UART, since the special characters
are not allowed for message tag and ID. 
*/
u32 uart_write (u8 *ptr) 
{
	u32 len = 0;
	
	if(*ptr == SERIAL_MSG_NEWLINE || *ptr == SERIAL_MSG_ESCAPE){
		set_char (SERIAL_MSG_ESCAPE);
		set_char (*ptr | SERIAL_MSG_MASK);
		len++;
	} else {
		for ( ; *ptr != '\0' ; len++) {
			set_char (*ptr++);
		}
	}

	return len;
}

/* Masks appropriate bytes before seding out.
 * Should be used by everyone that sends, unless
 * supressing the message masking is needed. 
 *
 * Call uart_set_char() to write directly and bypass masking.
 */
void uart_write_char (u8 ch) 
{
	u32 len = 0;
	
	if(ch == SERIAL_MSG_NEWLINE || ch == SERIAL_MSG_ESCAPE){
		set_char (SERIAL_MSG_ESCAPE);
		set_char (ch | SERIAL_MSG_MASK);
		len++;
	} else {
		uart_set_char (ch);
	}

	return;
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

// Sets the COMTX register for sending UART
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
		/*  output LineFeed for newline */
		COMTX = LF;
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
