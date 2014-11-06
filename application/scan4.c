#include "scan4.h"
#include "scan4_data.h"

#define S4_SEND_CNT 8

struct flash
{
	u32 adr;
	u16  buffer[PAGE_SIZE/2];		
};

struct act_pair
{
	dac a1;
	u16 a1_val;

	dac a2;
	u16 a2_val;
};

struct scan4 
{
	u8 ratio;
	u16 numPts;
	u16 numLines;
	struct flash f;
	struct act_pair xp;
	struct act_pair yp;
};

struct scan4 s4;

#define SET_PAIRX(v1, v2)	dac_set_val(s4.xp.a1, (s4.xp.a1_val = v1)); \
							dac_set_val(s4.xp.a2, (s4.xp.a2_val = v2))

#define SET_PAIRY(v1, v2)	dac_set_val(s4.yp.a1, (s4.yp.a1_val = v1)); \
							dac_set_val(s4.yp.a2, (s4.yp.a2_val = v2))

void scan4_get_data (void)
{
	u8 val_l, val_h;

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
	u8 val_l, val_h;

	for (i = 0; i < PAGE_SIZE/2; i ++)
	{
		val_l = uart_wait_get_char();
		val_h = (uart_wait_get_char() & 0x0F); 
		s4.f.buffer[i] = (val_h << 8) | val_l;	
	}

	if (flash_EraseSector (s4.f.adr))
		return false;
	if (flash_WriteAdr (s4.f.adr, PAGE_SIZE, (u8*)s4.f.buffer))
		return false;
	s4.f.adr += PAGE_SIZE;

	return true;	
}

void scan4_send_dac_data (void)
{
	u16 i, j;
	s4.f.adr = BLOCK0_BASE;
	
	for (i = 0; i < 16; i++)
	{
		flash_ReadAdr (s4.f.adr, PAGE_SIZE, (u8*)s4.f.buffer);
		for (j = 0; j < PAGE_SIZE/2; j++)
		{
			uart_set_char(s4.f.buffer[j]);
			uart_set_char(s4.f.buffer[j] >> 8);	
		}
		s4.f.adr += PAGE_SIZE;			
	}
}
				  
void scan4_start (void)
{
	SET_PAIRX(DAC_MAX, 0x0000);
	SET_PAIRY(DAC_MAX - (0x0FFF / s4.ratio), DAC_MAX / s4.ratio);
}

void scan4_step (void)
{
	u8 i;
	for (i = 0; i < S4_SEND_CNT; i++)
	{
		SET_PAIRX(s4.xp.a1_val--, s4.xp.a2_val++);
		SET_PAIRY(s4.yp.a1_val++, s4.yp.a2_val--);
	}
}

void scan4_init (void)
{
	s4.f.adr  = BLOCK0_BASE;
	s4.xp.a1  = DAC_X1;
	s4.xp.a2  = DAC_X2;
	s4.yp.a1  = DAC_Y1;
	s4.yp.a2  = DAC_Y2; 
}
