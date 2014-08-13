#ifndef __UART_H
#define __UART_H

#include "../global/global.h"

typedef enum {
	UART_OK,
	UART_RCV_ERROR,
	UBFR_OVERFLOW,
	UBFR_HW_OVERFLOW,
	UBFR_UNDERFLOW
} uart_status;

void uart_init (void);
void uart_buffered (bool enable);
void uart_handler (void);
u8 uart_get_num_bytes (void);

u32 uart_write (u8 * ptr);
u32 uart_write_bytes (u8* ptr, u32 size);

u8 uart_get_char (void);
u8 uart_wait_get_char (void);
u32 uart_wait_get_bytes (u8 * buffer, u32 num_bytes);

void uart_set_char (u8 ch);
	
void uart_set_num (s16);

bool is_received (void);

void wait_receive (void);

#endif
