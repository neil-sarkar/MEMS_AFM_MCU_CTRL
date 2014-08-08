#pragma once

#include "../global/global.h"
#include "../peripheral/pga_4ch.h"

typedef enum {
	wire3_ok,
	wire3_busy
} wire3_status;

void wire3_init (void);
void wire3_set_cpol (bool cpol);
void wire3_set_cs (volatile unsigned long * cs_dat_reg, u32 cs_bit);
void wire3_write_wait (u32 data, u8 len);
void wire3_handler (void);
wire3_status get_status (void);
