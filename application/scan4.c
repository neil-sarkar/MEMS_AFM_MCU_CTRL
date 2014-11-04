#include "scan4.h"
#include "scan4_data.h"

struct scan4 s4;

void scan4_get_data (void)
{
	u8 val_l, val_h;
	
//	scan4.dacTable = dacTable;

	val_l = uart_wait_get_char();
	s4.ratio = val_l;

	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	s4.numPts = (val_h << 8) | val_l;

	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	s4.numLines = (val_h << 8) | val_l;		
}

bool scan4_get_dac_data (void)
{
	u16 i;
	
	for (i = 0; i < PAGE_SIZE; i += 2)
	{
		s4.f.buffer[i+1] = uart_wait_get_char();
		s4.f.buffer[i] = (uart_wait_get_char() & 0x0F);		
	}

	if (flash_EraseSector (s4.f.adr))
		return false;
	if (flash_WriteAdr (s4.f.adr, PAGE_SIZE, (u8*)s4.f.buffer))
		return false;
	s4.f.adr += PAGE_SIZE;

	return true;	
}
				  
void scan4_start (void)
{
	
}

void scan4_step (void)
{
	
}
