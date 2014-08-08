#pragma once

#include "../global/global.h"

typedef enum {
	pga_fine,
	pga_dds,
	pga_x1,
	pga_x2,
	pga_y1,
	pga_y2
} pga;

void pga_4ch_init (void);
void pga_4ch_set (pga channel, u8 db);
