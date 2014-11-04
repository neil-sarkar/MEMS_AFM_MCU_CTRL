#pragma once

#include "../global/global.h"
#include "../peripheral/pga_4ch.h"
#include "../peripheral/uart.h"
#include "../peripheral/flash.h"

struct flash
{
	u32 adr;
	u8  buffer[PAGE_SIZE];		
};

struct scan4 
{
	u8 ratio;
	u16 numPts;
	u16 numLines;
	struct flash f;
};

void scan4_get_data (void);
void scan4_start (void);
void scan4_step (void);
bool scan4_get_dac_data (void);

